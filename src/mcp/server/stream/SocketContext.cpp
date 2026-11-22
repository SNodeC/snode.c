// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/stream/SocketContext.h"

#include "Log.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/stream/SocketConnection.h"
#include "mcp/server/Config.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
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
        bool inputEof = false, closing = false, failed = false, scheduled = false;
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
            if (owner != nullptr) {
                if (failed) {
                    owner->close();
                } else {
                    owner->shutdownWrite();
                }
            }
        }
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
                        config.getDeadline());
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
                receive("{");
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
        connection->setOnShutdown([weak = std::weak_ptr<State>(state)] {
            if (const auto owner = weak.lock()) {
                owner->finish();
            }
        });
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
    bool SocketContext::onSignal([[maybe_unused]] int signal) {
        state->finish();
        return true;
    }
} // namespace mcp::server::stream
