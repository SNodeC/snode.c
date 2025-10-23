/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef WEB_HTTP_LEGACY_IN_EVENTSOURCE_H
#define WEB_HTTP_LEGACY_IN_EVENTSOURCE_H

#include "web/http/legacy/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cctype>
#include <cstddef>
#include <list>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::in {

    using Client = web::http::legacy::in::Client;
    using MasterRequest = Client::MasterRequest;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketConnection = Client::SocketConnection;

    // JS-like readyState
    enum class ReadyState : int { CONNECTING = 0, OPEN = 1, CLOSED = 2 };

    // JS-like MessageEvent (simplified)
    struct MessageEvent {
        std::string type;        // "message" or custom from "event:"
        std::string data;        // joined data (LF between lines, trailing LF removed)
        std::string lastEventId; // from "id:"
        std::string origin;      // scheme://host[:port]
    };

    class EventSource : public std::enable_shared_from_this<EventSource> {
    private:
        using EventFn = std::function<void(const MessageEvent&)>;

        struct SharedState {
            std::list<EventFn> onMessageListener;
            std::map<std::string, std::list<EventFn>> onEventListener;
            std::function<void()> onOpen;
            std::function<void()> onError;

            std::string pending;
            std::string data;
            std::string type;
            std::string idBuf;
            std::string lastId;
            uint32_t retry = 3;

            ReadyState ready = ReadyState::CONNECTING;
            std::string scheme; // "http" or "https"
            std::string host;   // example.com
            uint16_t port = 80;
            std::string origin; // scheme://host[:port]
            std::string path;
            Client::Config* config;
        };

    public:
        EventSource(const EventSource&) = delete;
        EventSource& operator=(const EventSource&) = delete;
        EventSource(EventSource&&) noexcept = delete;
        EventSource& operator=(EventSource&&) noexcept = delete;

        ~EventSource() = default;

    private:
        EventSource()
            : state(std::make_shared<SharedState>()) {
        }

        void init(const std::string& url) {
            static const std::regex re(R"(^((https?)://)?([^/:]+)(?::(\d+))?(/.*)?$)");

            std::smatch match;
            if (std::regex_match(url, match, re)) {
                // capture URL parts for origin/ports
                state->scheme = (match[2].matched ? match[2].str() : "http");
                state->host = match[3].str();
                const bool isHttps = (state->scheme == "https");
                state->port =
                    match[4].matched ? static_cast<uint16_t>(std::stoi(match[4].str())) : static_cast<uint16_t>(isHttps ? 443 : 80);
                state->origin =
                    state->scheme + std::string("://") + state->host +
                    (((isHttps && state->port == 443) || (!isHttps && state->port == 80)) ? "" : (":" + std::to_string(state->port)));

                state->path = (match[5].matched ? match[5].str() : "/");

                LOG(TRACE) << "Full protocol: " << match[1];
                LOG(TRACE) << "     protocol: " << state->scheme;
                LOG(TRACE) << "         Host: " << state->host;
                LOG(TRACE) << "         Port: " << state->port;
                LOG(TRACE) << "         Path: " << state->path;
                LOG(TRACE) << "       Origin: " << state->origin;

                const std::weak_ptr<EventSource> eventStreamWeak = weak_from_this();

                client = std::make_shared<Client>(
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnect";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventSource> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = socketConnection;
                        }
                    },
                    [state = this->state](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnected";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
                    },
                    [eventStreamWeak, state = this->state](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnDisconnect";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventSource> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = nullptr;
                        }

                        if (state->config->getReconnect()) {
                            if (state->onError) {
                                state->onError();
                            }
                            state->ready = ReadyState::CONNECTING;
                        } else {
                            state->ready = ReadyState::CLOSED;
                        }
                    },
                    [url = match[5].matched ? match[5].str() : "/",
                     state = this->state](const std::shared_ptr<MasterRequest>& masterRequest) {
                        LOG(DEBUG) << masterRequest->getSocketContext()->getSocketConnection()->getConnectionName()
                                   << " EventSource: OnRequestStart";

                        masterRequest->requestEventSource(
                            url,
                            [masterRequest = std::weak_ptr<MasterRequest>(masterRequest), state]() -> std::size_t {
                                auto digits = [](std::string_view s) -> bool {
                                    for (const char c : s) {
                                        if (!std::isdigit(c)) {
                                            return false;
                                        }
                                    }

                                    return !s.empty();
                                };

                                auto parseU32 = [](std::string_view s) -> uint32_t {
                                    uint64_t v = 0;

                                    for (const char c : s) {
                                        v = v * 10 + (static_cast<unsigned char>(c) - '0');
                                        if (v > 0xFFFFFFFFULL) {
                                            return 0xFFFFFFFFU;
                                        }
                                    }

                                    return static_cast<uint32_t>(v);
                                };

                                auto deliverMessage =
                                    [state](const std::string& evType, const std::string& payload, const std::string& evId) {
                                        // 1) typed listeners (EventTarget semantics)
                                        if (auto it = state->onEventListener.find(evType); it != state->onEventListener.end()) {
                                            const MessageEvent e{evType, payload, evId, state->origin};
                                            for (const auto& onEventListener : it->second) {
                                                onEventListener(e);
                                            }
                                        }
                                        // 2) onmessage for default type
                                        if (evType == "message") {
                                            const MessageEvent e{evType, payload, evId, state->origin};

                                            for (const auto& onMessageListener : state->onMessageListener) {
                                                onMessageListener(e);
                                            }
                                        }
                                    };

                                auto dispatch = [state, &deliverMessage]() mutable {
                                    state->lastId = state->idBuf;
                                    if (state->data.empty()) {
                                        state->type.clear();
                                        return;
                                    }
                                    if (!state->data.empty() && state->data.back() == '\n') {
                                        state->data.pop_back();
                                    }

                                    const std::string evType = state->type.empty() ? "message" : std::exchange(state->type, std::string{});
                                    const std::string payload = std::exchange(state->data, std::string{});
                                    const std::string evId = state->lastId;

                                    deliverMessage(evType, payload, evId);
                                };

                                auto processLine = [state, &digits, &parseU32, &dispatch](std::string_view line) {
                                    if (line.empty()) {
                                        dispatch();
                                        return;
                                    }
                                    if (line.front() == ':') {
                                        return;
                                    }

                                    std::string_view f;
                                    std::string_view v;

                                    if (auto p = line.find(':'); p != std::string_view::npos) {
                                        f = line.substr(0, p);
                                        v = line.substr(p + 1);
                                        if (!v.empty() && v.front() == ' ') {
                                            v.remove_prefix(1);
                                        }
                                    } else {
                                        f = line;
                                    }

                                    if (f == "data") {
                                        state->data.append(v);
                                        state->data.push_back('\n');
                                    } else if (f == "event") {
                                        state->type = v;
                                    } else if (f == "id") {
                                        if (v.find('\0') == std::string_view::npos) {
                                            state->idBuf = v;
                                        }
                                    } else if (f == "retry") {
                                        if (digits(v)) {
                                            uint32_t const retry = parseU32(v);

                                            state->retry = retry;
                                            state->config->setReconnectTime(retry);
                                            state->config->setRetryTimeout(retry);
                                        }
                                    }
                                    // else ignore
                                };

                                auto parse = [state, &processLine]() {
                                    size_t start = 0;
                                    for (;;) {
                                        const size_t i = state->pending.find_first_of("\r\n", start);
                                        if (i == std::string::npos) {
                                            if (start) {
                                                state->pending.erase(0, start);
                                            }
                                            break;
                                        }

                                        if (state->pending[i] == '\r' && i + 1 >= state->pending.size()) {
                                            if (start) {
                                                state->pending.erase(0, start);
                                            }
                                            break;
                                        }

                                        const size_t eol =
                                            (state->pending[i] == '\r' && i + 1 < state->pending.size() && state->pending[i + 1] == '\n')
                                                ? 2
                                                : 1;

                                        const std::string_view line(&state->pending[start], i - start);
                                        start = i + eol;
                                        processLine(line);
                                    }
                                };

                                std::size_t consumed = 0;

                                if (!masterRequest.expired()) {
                                    char buf[16384];
                                    consumed = masterRequest.lock()->getSocketContext()->readFromPeer(buf, sizeof(buf));

                                    if (consumed > 0) {
                                        state->pending.append(buf, consumed);
                                        parse();
                                    }
                                } else {
                                    LOG(DEBUG) << "Server-sent event: server disconnect";
                                }

                                return consumed;
                            },
                            [masterRequestWeak = std::weak_ptr<MasterRequest>(masterRequest), state]() {
                                if (const std::shared_ptr<MasterRequest> masterRequest = masterRequestWeak.lock()) {
                                    LOG(DEBUG) << masterRequest->getSocketContext()->getSocketConnection()->getConnectionName()
                                               << ": server-sent event stream start";
                                }
                                state->onOpen();

                                state->config->setReconnectTime(state->retry);
                                state->config->setRetryTimeout(state->retry);

                                state->ready = ReadyState::OPEN;
                            },
                            [state]() {
                                LOG(DEBUG) << "Not an server-sent event endpoint: " << state->origin + state->path;
                            });
                    },
                    [](const std::shared_ptr<Request>& req) {
                        LOG(DEBUG) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " EventSource: OnRequestEnd";
                    });

                client->getConfig().Remote::setHost(state->host);
                client->getConfig().Remote::setPort(state->port);
                client->getConfig().setReconnect();
                client->getConfig().setRetry();
                client->getConfig().setRetryBase(1);

                state->config = &client->getConfig();

                client->connect([instanceName = client->getConfig().getInstanceName()](
                                    const core::socket::SocketAddress& socketAddress,
                                    const core::socket::State& state) { // example.com:81 simulate connnect timeout
                    switch (state) {
                        case core::socket::State::OK:
                            LOG(DEBUG) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                            break;
                        case core::socket::State::DISABLED:
                            LOG(DEBUG) << instanceName << ": disabled";
                            break;
                        case core::socket::State::ERROR:
                            LOG(DEBUG) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                            break;
                        case core::socket::State::FATAL:
                            LOG(DEBUG) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                            break;
                    }
                });
            }
        }

    public:
        EventSource* onMessage(EventFn fn) {
            state->onMessageListener.push_back(std::move(fn));

            return this;
        }

        EventSource* addEventListener(const std::string& key, EventFn fn) {
            state->onEventListener[key].push_back(std::move(fn));

            return this;
        }

        EventSource* removeEventListeners(const std::string& type) {
            state->onEventListener.erase(type);

            return this;
        }

        void close() {
            client->getConfig().setReconnect(false);

            if (socketConnection != nullptr) {
                socketConnection->shutdownRead();
            }
        }

        EventSource* onOpen(std::function<void()> h) {
            state->onOpen = std::move(h);

            return this;
        }

        EventSource* onError(std::function<void()> h) {
            state->onError = std::move(h);

            return this;
        }

        ReadyState readyState() const {
            return state->ready;
        }

        const std::string& lastEventId() const {
            return state->lastId;
        }

        uint32_t retry() const {
            return state->retry;
        }

        EventSource* retry(uint32_t retry) {
            state->retry = retry;
            state->config->setReconnectTime(retry);
            state->config->setRetryTimeout(retry);

            return this;
        }

        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<SharedState> state;

        friend inline std::shared_ptr<class EventSource> EventSource(const std::string& url);
    };

    inline std::shared_ptr<class EventSource> EventSource(const std::string& url) {
        std::shared_ptr<class EventSource> eventSource = std::shared_ptr<class EventSource>(new class EventSource());
        eventSource->init(url);
        return eventSource;
    }

} // namespace web::http::legacy::in

#endif // WEB_HTTP_LEGACY_IN_EVENTSOURCE_H
