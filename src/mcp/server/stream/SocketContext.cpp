/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mcp/server/stream/SocketContext.h"

#include "Log.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/stream/SocketConnection.h"
#include "mcp/server/Config.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server::stream {

    using namespace mcp::server::detail;
    using WireReply = Access::Response;
    using WireServer = Access::Registry;

    struct SocketContext::State : std::enable_shared_from_this<State> {
        SocketContext* owner;
        Server& server;
        Config& config;
        std::string incoming;
        std::map<std::string, std::shared_ptr<WireReply>> pending;
        bool inputEof = false;
        bool closing = false;
        bool failed = false;
        bool scheduled = false;
        std::shared_ptr<Channel> channel = std::make_shared<Channel>();
        ProtocolEra era = ProtocolEra::PerRequest20260728;

        State(SocketContext* owner, Server& server, Config& config)
            : owner(owner)
            , server(server)
            , config(config) {
        }

        void send(const Json& value);
        void receive(const std::string& line);
        void process();
        void finish();
        void parseError();
    };

    void SocketContext::State::send(const Json& value) {
        if (owner != nullptr && !value.is_null() && !failed) {
            const std::string bytes = value.dump() + "\n";
            const auto* connection = owner->getSocketConnection();
            const auto queued = connection->getTotalQueued() - connection->getTotalSent();
            if (bytes.size() > config.getMaximumOutputBytes() - std::min(queued, config.getMaximumOutputBytes())) {
                snode::log::framework("mcp.server", snode::log::Boundary::Context).error() << "stdio output limit exceeded";
                failed = true;
                finish();
            } else {
                static_cast<void>(owner->sendToPeer(bytes));
            }
        }
    }

    void SocketContext::State::finish() {
        if (!closing) {
            closing = true;
            for (const auto& [id, reply] : pending) {
                static_cast<void>(id);
                reply->cancel(CancelReason::Shutdown);
            }

            pending.clear();
            channel->active = false;
            channel->resources.clear();
            channel->output = {};
            if (owner != nullptr) {
                if (failed) {
                    owner->close();
                } else {
                    owner->shutdownWrite();
                }
            }
        }
    }

    void SocketContext::State::parseError() {
        WireReply(
            nullptr,
            era,
            CachePolicy::None,
            [this](const Json& value, bool) {
                send(value);
            },
            config.getDeadline())
            .error(-32700, "Parse error");
    }

    void SocketContext::State::receive(const std::string& line) {
        std::erase_if(pending, [](const auto& item) {
            return item.second->done;
        });
        Json request;
        bool parsed = false;
        try {
            request = Json::parse(line);
            parsed = true;
        } catch (const Json::exception&) {
            parseError();
        }
        if (parsed) {
            const bool hasMeta = request.is_object() && request.contains("params") && request["params"].is_object() &&
                                 request["params"].contains("_meta") && request["params"]["_meta"].is_object() &&
                                 (request["params"]["_meta"].contains(versionKey) || request["params"]["_meta"].contains(capabilitiesKey));
            const std::string method =
                request.is_object() && request.value("method", Json()).is_string() ? request["method"].get<std::string>() : "";
            const bool initializing = method == "initialize" && !hasMeta;
            const ProtocolEra requestEra = (era == ProtocolEra::Initialization20251125 || initializing) && !hasMeta
                                               ? ProtocolEra::Initialization20251125
                                               : ProtocolEra::PerRequest20260728;
            if (method == "notifications/cancelled" && WireServer::validRequest(request) && !request.contains("id") &&
                request.value("params", Json()).is_object()) {
                const std::string key = request["params"].value("requestId", Json()).dump();
                if (pending.contains(key)) {
                    pending[key]->cancel();
                }
            } else {
                const Json id = request.is_object() ? request.value("id", Json()) : Json();
                const std::string key = id.dump();
                if ((!id.is_null() && pending.contains(key)) || pending.size() >= 64) {
                    WireReply(
                        id,
                        requestEra,
                        CachePolicy::None,
                        [this](const Json& value, bool) {
                            send(value);
                        },
                        config.getDeadline())
                        .error(-32600, "Duplicate id or in-flight limit");
                } else {
                    const std::weak_ptr<State> weak = shared_from_this();
                    channel->output = [weak](const Json& value) {
                        if (const auto current = weak.lock()) {
                            current->send(value);
                        }
                    };
                    auto reply = Access::registry(server).dispatch(
                        request,
                        requestEra,
                        [weak, initializing](const Json& value, bool) {
                            if (const auto owner = weak.lock()) {
                                if (initializing && value.contains("result")) {
                                    owner->era = ProtocolEra::Initialization20251125;
                                }

                                owner->send(value);
                            }
                        },
                        config.getDeadline(),
                        {},
                        channel);
                    if (!reply->done) {
                        pending[key] = std::move(reply);
                    }
                }
            }
        }
    }

    void SocketContext::State::process() {
        std::size_t count = 0;
        std::size_t newline = incoming.find('\n');
        while (!closing && count < 8 && newline != std::string::npos) {
            if (newline > config.getMaximumMessageBytes()) {
                failed = true;
                finish();
            } else {
                const std::string line = incoming.substr(0, newline);
                incoming.erase(0, newline + 1);
                receive(line);
                ++count;
                newline = incoming.find('\n');
            }
        }

        if (!closing && newline != std::string::npos && !scheduled) {
            scheduled = true;
            const std::weak_ptr<State> weak = shared_from_this();
            core::EventReceiver::atNextTick([weak] {
                if (const auto state = weak.lock()) {
                    state->scheduled = false;
                    if (core::SNodeC::state() == core::State::RUNNING) {
                        state->process();
                    }
                }
            });
        }

        if (!closing && inputEof && newline == std::string::npos) {
            if (!incoming.empty()) {
                parseError();
                incoming.clear();
            }

            finish();
        }

        if (!closing && incoming.size() > config.getMaximumMessageBytes() && newline == std::string::npos) {
            failed = true;
            finish();
        }
    }

    SocketContext::SocketContext(core::socket::stream::SocketConnection* connection, Server& server, Config& config)
        : core::socket::stream::SocketContext(connection)
        , state(std::make_shared<State>(this, server, config)) {
    }

    SocketContext::~SocketContext() = default;

    void SocketContext::onConnected() {
    }

    void SocketContext::onDisconnected() {
        state->finish();
        state->owner = nullptr;
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        char bytes[16384];
        const auto count = readFromPeer(bytes, sizeof(bytes));
        if (!state->closing) {
            state->incoming.append(bytes, count);
            if (state->incoming.size() > state->config.getMaximumMessageBytes() + sizeof(bytes)) {
                state->failed = true;
                state->finish();
            } else {
                state->process();
            }
        }

        return count;
    }

    void SocketContext::onReadError(int error) {
        if (error == 0) {
            state->inputEof = true;
            state->process();
        } else {
            state->finish();
        }
    }

    void SocketContext::onWriteError([[maybe_unused]] int error) {
        state->failed = true;
        state->finish();
    }

    void SocketContext::onShutdown() {
        state->finish();
    }

    bool SocketContext::onSignal([[maybe_unused]] int signal) {
        state->finish();

        return true;
    }

} // namespace mcp::server::stream
