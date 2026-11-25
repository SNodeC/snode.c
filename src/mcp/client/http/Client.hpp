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

#pragma once
#include "core/timer/Timer.h"
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/client/Mcp.h" // IWYU pragma: keep
#include "mcp/client/http/Client.h"
#include "mcp/detail/Headers.h"
#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <cstddef>
#include <deque>
#include <exception>
#include <list>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <utility>
#endif
namespace mcp::client::http {
    namespace detail {
        class Events final : public web::http::client::tools::EventSource {
        public:
            ~Events() final;

            bool consume(std::string_view bytes) {
                return feed(sharedState, bytes);
            }

            void reset() {
                sharedState->onMessageListener.clear();
                sharedState->pending.clear();
                sharedState->firstLine = true;
                sharedState->skipLf = false;
                sharedState->data.clear();
                sharedState->type.clear();
                sharedState->idBuf = sharedState->lastId;
            }

            void close() final {
                sharedState->ready = ReadyState::CLOSED;
            }
        };

    } // namespace detail

    template <typename Transport>
    struct Client<Transport>::State final
        : jsonrpc::JsonRpcContext
        , std::enable_shared_from_this<State> {
        using Request = web::http::client::MasterRequest;
        using Response = web::http::client::Response;

        struct Exchange {
            std::string body;
            Json id;
            std::string method;
            detail::Events events;
            std::string json;
            bool get = false;
            bool sse = false;
            bool completed = false;
            std::string status;

            bool accepted() const {
                return status == "200" || status == "202" || status == "204" || (get && status == "405");
            }

            unsigned retries = 0;
            Request* active = nullptr;
            core::timer::Timer retry;

            ~Exchange() {
                retry.cancel();
            }
        };

        State(Mcp* protocol, Options options)
            : JsonRpcContext(protocol)
            , options(std::move(options)) {
        }

        ~State() final {
            stop();
        }

        Mcp& protocol() const {
            return *static_cast<Mcp*>(jsonRpc);
        }

        void init(const std::string& name) {
            const auto weak = this->weak_from_this();
            transport = std::make_unique<Transport>(
                name,
                [weak](const std::shared_ptr<Request>& request) {
                    if (const auto self = weak.lock(); self && !self->queue.empty()) {
                        const auto exchange = self->queue.front();
                        self->queue.pop_front();
                        self->active.emplace(request.get(), exchange);
                        try {
                            self->deliver(request, exchange);
                        } catch (const std::exception&) {
                            exchange->completed = true;
                            self->report("Invalid HTTP client request configuration");
                            if (!exchange->id.is_null()) {
                                self->onMessage(
                                    jsonrpc::messages::Error{exchange->id, -32602, "Invalid HTTP request configuration"}.serialize());
                            }
                            request->getSocketContext()->close();
                        }
                    } else {
                        request->getSocketContext()->close();
                    }
                },
                [weak](const std::shared_ptr<Request>& request) {
                    if (const auto self = weak.lock()) {
                        self->disconnected(request.get());
                    }
                });
            transport->getConfig()->net::config::ConfigInstance::forceUnrequired();
            transport->getConfig()->setReconnect(false);
            transport->getConfig()->setRetry(false);
        }

        void report(const std::string& message) {
            if (options.onError) {
                options.onError(message);
            }
        }

        void connect() {
            transport->connect(address, [weak = this->weak_from_this()](const auto&, const core::socket::State& status) {
                if (const auto self = weak.lock(); self && status != core::socket::State::OK) {
                    self->report("MCP HTTP connection failed: " + status.what());
                    self->stop();
                }
            });
        }

        void release() {
            if (!notifications) {
                notifications = true;
                const auto count = queue.size();
                for (std::size_t index = 0; index < count; ++index) {
                    connect();
                }
            }
        }

        void enqueue(const std::shared_ptr<Exchange>& exchange) {
            if (!stopped && active.size() + queue.size() < 64) {
                exchange->events.reset();
                exchange->json.clear();
                if (exchange->get && !notifications) {
                    queue.push_front(exchange);
                } else {
                    queue.push_back(exchange);
                }
                if (protocol().getProtocolEra() != ProtocolEra::Initialization20251125 || notifications || exchange->get ||
                    exchange->method == "initialize" || exchange->method == "notifications/initialized") {
                    connect();
                }
            } else {
                report("MCP HTTP exchange limit or closed client");
            }
        }

        void deliver(const std::shared_ptr<Request>& request, const std::shared_ptr<Exchange>& exchange) {
            exchange->active = request.get();
            request->method = exchange->get ? "GET" : "POST";
            request->url = options.path;
            request->set("Accept", "application/json, text/event-stream");
            request->set("Content-Type", "application/json");
            request->set("Connection", "keep-alive");
            request->set("MCP-Protocol-Version",
                         protocol().getProtocolEra() == ProtocolEra::Initialization20251125 ? protocolVersion2025 : protocolVersion2026);
            if (!session.empty()) {
                request->set("Mcp-Session-Id", session);
            }
            if (!exchange->events.lastEventId().empty()) {
                request->set("Last-Event-ID", exchange->events.lastEventId());
            }
            if (options.bearerToken) {
                const std::string token = options.bearerToken();
                if (token.find_first_of("\r\n") != std::string::npos) {
                    report("Invalid bearer token");
                    request->getSocketContext()->close();
                } else if (!token.empty()) {
                    request->set("Authorization", "Bearer " + token);
                }
            }
            if (!exchange->get && protocol().getProtocolEra() == ProtocolEra::PerRequest20260728) {
                const Json message = Json::parse(exchange->body);
                const Json params = message.value("params", Json::object());
                request->set("Mcp-Method", exchange->method);
                const std::string key = exchange->method == "resources/read"     ? "uri"
                                        : exchange->method.starts_with("tasks/") ? "taskId"
                                                                                 : "name";
                if (params.contains(key)) {
                    request->set("Mcp-Name", mcp::detail::encodeHeader(params[key]));
                }
                if (exchange->method == "tools/call") {
                    const auto headers = toolHeaders.find(params.value("name", ""));
                    if (headers != toolHeaders.end()) {
                        for (const auto& [name, value] :
                             mcp::detail::parameterHeaders(headers->second, params.value("arguments", Json::object()))) {
                            request->set(name, value);
                        }
                    }
                }
            }
            const auto weak = this->weak_from_this();
            exchange->events.onMessage([weak, weakExchange = std::weak_ptr<Exchange>(exchange)](const auto& event) {
                if (const auto self = weak.lock()) {
                    if (const auto current = weakExchange.lock(); current && !event.data.empty()) {
                        self->message(current, event.data);
                    }
                }
            });
            request->stream(
                exchange->get ? std::string{} : exchange->body,
                [weak, exchange](const std::shared_ptr<Response>& response) {
                    if (const auto self = weak.lock()) {
                        exchange->status = response->statusCode;
                        exchange->sse = web::http::ciContains(response->get("Content-Type"), "text/event-stream");
                        if (!response->get("Mcp-Session-Id").empty()) {
                            self->session = response->get("Mcp-Session-Id");
                        }
                        if (exchange->get && !self->notifications) {
                            if (response->statusCode == "200" || response->statusCode == "405") {
                                self->release();
                            } else {
                                self->report("MCP notification channel rejected");
                                self->stop();
                            }
                        }
                        if (response->statusCode == "410" || response->statusCode == "409") {
                            exchange->completed = true;
                            self->report("MCP replay unavailable: HTTP " + response->statusCode);
                        }
                    }
                },
                [weak, exchange](std::string_view bytes) {
                    if (const auto self = weak.lock()) {
                        if (exchange->sse) {
                            if (!exchange->events.consume(bytes)) {
                                self->report("Invalid or oversized SSE event");
                                exchange->completed = true;
                            }
                        } else if (exchange->json.size() + bytes.size() <= 8 * 1024 * 1024) {
                            exchange->json.append(bytes);
                        } else {
                            self->report("Oversized MCP HTTP response");
                            exchange->completed = true;
                        }
                    }
                },
                [weak, exchange]() {
                    if (const auto self = weak.lock()) {
                        if (!exchange->sse) {
                            if (!exchange->json.empty()) {
                                self->message(exchange, exchange->json);
                            } else if (!exchange->accepted() || (!exchange->id.is_null() && !exchange->method.empty())) {
                                if (exchange->method == "server/discover" && (exchange->status == "404" || exchange->status == "405")) {
                                    self->onMessage(
                                        jsonrpc::messages::Error{exchange->id, -32601, "Discovery endpoint unavailable"}.serialize());
                                } else {
                                    self->report("MCP HTTP response rejected: " + exchange->status);
                                    self->stop();
                                }
                            }
                            exchange->completed = true;
                        }
                        if (!self->stopped && exchange->method == "notifications/initialized" && exchange->accepted()) {
                            if (self->session.empty()) {
                                self->release();
                            } else {
                                const auto stream = std::make_shared<Exchange>();
                                stream->get = true;
                                self->enqueue(stream);
                            }
                        }
                        if (exchange->completed && exchange->active != nullptr) {
                            exchange->active->getSocketContext()->close();
                        }
                    }
                },
                [weak, exchange](const std::string& error) {
                    if (const auto self = weak.lock()) {
                        self->report(error);
                        exchange->completed = true;
                    }
                });
        }

        void message(const std::shared_ptr<Exchange>& exchange, const std::string& text) {
            Json value = Json::parse(text, nullptr, false);
            if (value.is_discarded()) {
                report("Invalid MCP HTTP JSON");
                stop();
            } else {
                const bool response = value.contains("id") && !value.contains("method") && value["id"] == exchange->id;
                if (exchange->method == "tools/list" && value.contains("result") && value["result"].contains("tools")) {
                    Json valid = Json::array();
                    for (const auto& tool : value["result"]["tools"]) {
                        const auto parameters = mcp::detail::headerParameters(tool.value("inputSchema", Json::object()));
                        if (parameters && toolHeaders.size() < 16384) {
                            toolHeaders[tool.at("name").template get<std::string>()] = *parameters;
                            valid.push_back(tool);
                        } else {
                            report("Excluded tool with invalid HTTP header annotations");
                        }
                    }
                    value["result"]["tools"] = std::move(valid);
                }
                if (value.value("method", "") == "notifications/tools/list_changed") {
                    toolHeaders.clear();
                }
                onMessage(value.dump());
                if (response) {
                    exchange->completed = true;
                    if (exchange->active != nullptr) {
                        exchange->active->getSocketContext()->close();
                    }
                }
            }
        }

        void disconnected(Request* request) {
            const auto it = active.find(request);
            if (it != active.end()) {
                const auto exchange = it->second;
                active.erase(it);
                exchange->active = nullptr;
                if (!stopped && !exchange->completed && protocol().getProtocolEra() == ProtocolEra::Initialization20251125 &&
                    !exchange->events.lastEventId().empty() && exchange->retries < 8) {
                    exchange->get = true;
                    ++exchange->retries;
                    exchange->retry = core::timer::Timer::singleshotTimer(
                        [weak = this->weak_from_this(), exchange]() {
                            auto timer = std::move(exchange->retry);
                            if (const auto self = weak.lock()) {
                                self->enqueue(exchange);
                            }
                        },
                        utils::Timeval(std::max(1U, exchange->events.retry()) / 1000.));
                } else if (!stopped && !exchange->completed && !exchange->id.is_null()) {
                    report("MCP HTTP stream disconnected");
                }
            }
        }

        void sendMessage(const std::string& text) final {
            const Json value = Json::parse(text);
            const std::string method = value.value("method", std::string{});
            if (method == "notifications/cancelled" && protocol().getProtocolEra() == ProtocolEra::PerRequest20260728) {
                const Json id = value.value("params", Json::object()).value("requestId", Json());
                for (auto& [request, exchange] : active) {
                    if (exchange->id == id) {
                        exchange->completed = true;
                        request->getSocketContext()->close();
                    }
                }
            } else {
                const auto exchange = std::make_shared<Exchange>();
                exchange->body = text;
                exchange->id = value.value("id", Json());
                exchange->method = method;
                enqueue(exchange);
            }
        }

        void stop() {
            if (!stopped) {
                stopped = true;
                for (auto& [request, exchange] : active) {
                    exchange->retry.cancel();
                    request->getSocketContext()->close();
                }
                for (const auto& exchange : queue) {
                    exchange->retry.cancel();
                }
                queue.clear();
                if (connected) {
                    connected = false;
                    onDisconnected();
                }
            }
        }

        void end() final {
            stop();
        }

        void close() final {
            stop();
        }

        core::socket::stream::SocketConnection* getSocketConnection() const final {
            return nullptr;
        }

        Options options;
        std::unique_ptr<Transport> transport;
        SocketAddress address;
        std::deque<std::shared_ptr<Exchange>> queue;
        std::map<Request*, std::shared_ptr<Exchange>> active;
        std::string session;
        std::map<std::string, mcp::detail::HeaderParameters> toolHeaders;
        bool notifications = false;
        bool stopped = false;
        bool connected = false;
    };

    template <typename Transport>
    Client<Transport>::Client(const std::string& name, Mcp* protocol, Options options)
        : state(std::make_shared<State>(protocol, std::move(options))) {
        state->init(name);
    }

    template <typename Transport>
    Client<Transport>::~Client() {
        stop();
    }

    template <typename Transport>
    typename Client<Transport>::Config* Client<Transport>::getConfig() const {
        return state->transport->getConfig();
    }

    template <typename Transport>
    void Client<Transport>::setPath(const std::string& path) {
        if (!state->connected && path.starts_with('/') && path.find_first_of("\r\n") == std::string::npos) {
            state->options.path = path;
        }
    }

    template <typename Transport>
    void Client<Transport>::start(const SocketAddress& address) {
        state->address = address;
        core::EventReceiver::atNextTick([state = state]() {
            if (!state->connected && !state->stopped) {
                state->connected = true;
                state->onConnected();
            }
        });
    }

    template <typename Transport>
    void Client<Transport>::stop() {
        state->stop();
    }
} // namespace mcp::client::http
