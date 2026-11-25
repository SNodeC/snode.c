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

#include "mcp/server/Mcp.h"

#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/ErrorCode.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "mcp/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <set>
#include <utility>
#include <vector>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mcp::server {

    using namespace detail;

    struct Mcp::State {
        Mcp* owner;
        const detail::Access::Registry& registry;
        utils::Timeval timeout;
        std::shared_ptr<Channel> channel;
        std::optional<ProtocolEra> fixedEra;
        ProtocolEra era = ProtocolEra::PerRequest20260728;
        std::optional<Principal> principal;
        std::map<Json, std::shared_ptr<Response>> pending;
        std::shared_ptr<Response> reply;
        Response::Output sink;
        bool accepted = false;
        bool closing = false;
    };

    namespace {

        class Exchange final : public jsonrpc::JsonRpcContext {
        public:
            explicit Exchange(Mcp* peer)
                : JsonRpcContext(peer)
                , peer(peer) {
            }

            ~Exchange() final;

            void sendMessage(const std::string& message) final {
                peer->deliver(message);
            }

            void end() final {
            }

            void close() final {
            }

            core::socket::stream::SocketConnection* getSocketConnection() const final {
                return nullptr;
            }

        private:
            Mcp* peer;
        };

        Exchange::~Exchange() = default;

    } // namespace

    Mcp::Mcp(const detail::Access::Registry& registry,
             const utils::Timeval& timeout,
             std::shared_ptr<Channel> channel,
             std::optional<ProtocolEra> era,
             std::optional<Principal> principal,
             std::size_t maximumMessageSize)
        : Super("mcp")
        , state(std::make_shared<State>(State{this,
                                              registry,
                                              timeout,
                                              channel ? std::move(channel) : (!era ? std::make_shared<Channel>() : nullptr),
                                              era,
                                              era.value_or(ProtocolEra::PerRequest20260728),
                                              std::move(principal),
                                              {},
                                              {},
                                              {},
                                              false,
                                              false})) {
        setBatchesEnabled(false);
        setMaximumMessageSize(maximumMessageSize);
    }

    Mcp::~Mcp() {
        state->owner = nullptr;
        finish();
    }

    void Mcp::finish() {
        if (!state->closing) {
            state->closing = true;
            auto pending = std::move(state->pending);
            for (const auto& [id, reply] : pending) {
                static_cast<void>(id);
                reply->cancel(CancelReason::Shutdown);
            }
        }
    }

    void Mcp::onConnected() {
        Super::onConnected();
        const std::weak_ptr<State> weak = state;
        state->channel->output = [weak](const Json& value) {
            if (const auto current = weak.lock(); current && current->owner != nullptr && !current->closing && !value.is_null()) {
                current->owner->sendNotification(value["method"].get<std::string>(), value.value("params", Json::object()));
            }
        };
    }

    void Mcp::onDisconnected() {
        finish();
        if (!state->fixedEra) {
            state->channel->active = false;
            state->channel->resources.clear();
            state->channel->output = {};
        }
        Super::onDisconnected();
    }

    bool Mcp::onSignal([[maybe_unused]] int sig) {
        finish();
        return true;
    }

    bool Mcp::validRequest(const Json& request) {
        return Super::isRequest(request) && request["method"] != "" && (!request.contains("id") || !request["id"].is_null()) &&
               (!request.contains("params") || request["params"].is_object());
    }

    Json Mcp::errorResponse(const Json& id, ProtocolEra era, int code, const std::string& message, Json data) {
        Json value =
            jsonrpc::messages::Error{id, code, message, data.is_null() ? std::nullopt : std::optional<Json>(std::move(data))}.toJson();
        return wire(std::move(value), era);
    }

    bool Server::Impl::validRequest(const Json& request) {
        return Mcp::validRequest(request);
    }

    Json Mcp::wire(Json value) const {
        return wire(std::move(value), state->era);
    }

    Json Mcp::wire(Json value, ProtocolEra era) {
        if (era == ProtocolEra::PerRequest20260728 && value.contains("error") && value.value("id", Json()).is_null()) {
            value.erase("id");
        }
        return value;
    }

    void Mcp::deliver(const std::string& message) {
        if (state->sink) {
            Json value = wire(Json::parse(message));
            const bool terminal = value.contains("result") || value.contains("error");
            state->sink(value, terminal);
        }
    }

    std::shared_ptr<Response> Mcp::receive(const Json& request, Response::Output sink) {
        state->sink = std::move(sink);
        jsonRpcContext->onMessage(request.dump());
        auto reply = std::move(state->reply);
        if (reply) {
            state->pending.erase(reply->id);
        } else {
            reply = std::make_shared<Response>(nullptr, state->era, CachePolicy::None, Response::Output{}, state->timeout);
            reply->done = true;
        }
        return reply;
    }

    void Mcp::onRequest(const jsonrpc::messages::Request& request) {
        std::erase_if(state->pending, [](const auto& entry) {
            return entry.second->done;
        });
        const bool valid = validRequest(request.toJson());
        const Json params = request.params.value_or(Json::object());
        const Json meta = params.is_object() ? params.value("_meta", Json()) : Json();
        const bool hasMeta = meta.is_object() && (meta.contains(versionKey) || meta.contains(capabilitiesKey));
        const bool initializing = request.method == "initialize" && !hasMeta;
        const ProtocolEra era = state->fixedEra.value_or((state->era == ProtocolEra::Initialization20251125 || initializing) && !hasMeta
                                                             ? ProtocolEra::Initialization20251125
                                                             : ProtocolEra::PerRequest20260728);
        const std::weak_ptr<State> weak = state;
        const Response::Output sink = state->sink;
        const auto output = [weak, sink, id = request.id, initializing](const Json& value, bool terminal) {
            if (const auto current = weak.lock(); current && current->owner != nullptr) {
                if (initializing && value.contains("result")) {
                    current->era = ProtocolEra::Initialization20251125;
                }
                current->owner->output(value, terminal, sink, id);
            }
        };
        std::shared_ptr<Response> reply;
        if (!valid || state->pending.size() >= 64) {
            reply = std::make_shared<Response>(request.id, era, CachePolicy::None, output, state->timeout);
            reply->error(jsonrpc::messages::InvalidRequest, !valid ? "Invalid Request" : "Duplicate id or in-flight limit");
        } else {
            reply = state->registry.dispatchRequest(request, era, output, state->timeout, state->principal, state->channel, this);
        }
        if (!reply->done) {
            state->pending.emplace(request.id, reply);
        }
        if (state->sink) {
            state->reply = std::move(reply);
        }
    }

    void Mcp::output(const Json& value, bool terminal, const Response::Output& sink, const Json& id) {
        // HTTP's typed callback retains its JSON number types across wire serialization.
        const Response::Output typed = sink ? Response::Output([sink, value](const Json&, bool done) {
            sink(value, done);
        })
                                            : sink;
        const auto previous = std::exchange(state->sink, typed);
        if (value.is_null()) {
            discardRequest(id);
            if (sink) {
                sink(nullptr, terminal);
            }
        } else if (value.contains("result")) {
            sendResult(value["id"], value["result"]);
        } else if (value.contains("error")) {
            const auto& error = value["error"];
            sendError(id,
                      error["code"].get<std::int64_t>(),
                      error["message"].get<std::string>(),
                      error.contains("data") ? std::optional<Json>(error["data"]) : std::nullopt);
        } else if (value.contains("id")) {
            sendRequest(jsonrpc::messages::Request{value["id"], value["method"].get<std::string>(), std::optional<Json>(value["params"])});
        } else {
            sendNotification(value["method"].get<std::string>(), value.value("params", Json::object()));
        }
        state->sink = previous;
        if (terminal) {
            state->pending.erase(id);
        }
    }

    void Mcp::onNotification(const jsonrpc::messages::Notification& notification) {
        if (notification.method == "notifications/cancelled" && notification.params && notification.params->is_object()) {
            const auto found = state->pending.find(notification.params->value("requestId", Json()));
            if (found != state->pending.end()) {
                const auto reply = found->second;
                reply->cancel();
            }
        }
    }

    bool Mcp::validateResult(const jsonrpc::messages::Result& result) const {
        bool accepted = false;
        if (state->channel && result.result.is_object()) {
            const auto found = state->channel->inputs.find(result.id.dump());
            if (found != state->channel->inputs.end()) {
                const Json action = result.result.value("action", Json());
                const Json content = result.result.value("content", Json());
                const bool validContent = content.is_null() || content.is_object();
                accepted = (action == "accept" && (!found->second.requestedSchema.is_null() || validContent)) ||
                           ((action == "decline" || action == "cancel") && validContent);
            }
        }
        return accepted;
    }

    void Mcp::onResult(const jsonrpc::messages::Result& result) {
        state->accepted = state->channel && state->channel->complete(result.toJson());
    }

    void Mcp::onError(const jsonrpc::messages::Error& error) {
        state->accepted = state->channel && state->channel->complete(error.toJson());
    }

    bool Mcp::respond(const Json& message) {
        state->accepted = false;
        jsonRpcContext->onMessage(message.dump());
        return state->accepted;
    }

    void Mcp::forget(const Json& id) {
        cancelRequest(id);
    }

    std::shared_ptr<detail::Access::Response> Server::Impl::dispatch(const Json& request,
                                                                     ProtocolEra era,
                                                                     Response::Output output,
                                                                     const utils::Timeval& timeout,
                                                                     std::optional<Principal> principal,
                                                                     std::shared_ptr<Channel> channel) const {
        auto* peer = new Mcp(*this, timeout, std::move(channel), era, std::move(principal));
        const auto context = std::make_shared<Exchange>(peer);
        auto reply = peer->receive(request, std::move(output));
        reply->protocolOwner = context;
        return reply;
    }

    bool detail::Channel::respond(const Json& message) {
        bool accepted = false;
        if (active && message.is_object()) {
            const auto found = inputs.find(message.value("id", Json()).dump());
            if (found != inputs.end()) {
                if (const auto response = found->second.response.lock(); response && response->protocol != nullptr) {
                    const auto owner = response->protocolOwner;
                    accepted = response->protocol->respond(message);
                } else {
                    inputs.erase(found);
                }
            }
        }
        return accepted;
    }

    bool detail::Channel::complete(const Json& message) {
        bool accepted = false;
        if (active) {
            const auto found = inputs.find(message["id"].dump());
            if (found != inputs.end()) {
                const auto response = found->second.response.lock();
                const std::string key = found->second.key;
                if (response) {
                    const Json result = message.value("result", Json());
                    const Json failure = message.value("error", Json());
                    const Json action = result.is_object() ? result.value("action", Json()) : Json();
                    const Json content = result.is_object() ? result.value("content", Json()) : Json();
                    bool failed = !failure.is_null();
                    std::string validationError;
                    if (action == "accept" && !found->second.requestedSchema.is_null()) {
                        failed = !Access::Registry::validate(found->second.requestedSchema, content, validationError) || failed;
                    }

                    response->expire();
                    if (!response->done) {
                        accepted = true;
                        inputs.erase(message["id"].dump());
                        if (failed) {
                            if (response->toolRequest) {
                                response->result(
                                    {{"isError", true}, {"content", {{{"type", "text"}, {"text", "Client elicitation failed"}}}}});
                            } else {
                                response->error(-32603, "Client elicitation failed");
                            }
                        } else {
                            response->inputResponses[key] = InputResponse{action == "accept"    ? ElicitationAction::Accept
                                                                          : action == "decline" ? ElicitationAction::Decline
                                                                                                : ElicitationAction::Cancel,
                                                                          content};
                            if (std::none_of(inputs.begin(), inputs.end(), [&response](const auto& item) {
                                    return item.second.response.lock() == response;
                                })) {
                                const auto resume = response->resume;
                                resume();
                            }
                        }
                    }
                } else {
                    inputs.erase(found);
                }
            }
        }

        return accepted;
    }

    void detail::Channel::forget(const Response& response) {
        std::vector<Json> ids;
        std::erase_if(inputs, [&response, &ids](const auto& item) {
            const auto current = item.second.response.lock();
            const bool remove = !current || current.get() == &response;
            if (current && remove) {
                ids.push_back(Json::parse(item.first));
            }

            return remove;
        });
        if (active) {
            for (const auto& id : ids) {
                if (response.protocol != nullptr) {
                    response.protocol->forget(id);
                }
                response.output(
                    jsonrpc::messages::Notification{"notifications/cancelled",
                                                    std::optional<Json>(Json{{"requestId", id}, {"reason", "Originating request ended"}})}
                        .toJson(),
                    false);
            }
        }
    }

} // namespace mcp::server
