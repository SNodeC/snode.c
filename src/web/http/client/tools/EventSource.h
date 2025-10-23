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

#ifndef WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
#define WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H

#include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/State.h"
#include "log/Logger.h"

#include <cctype>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <stdint.h>
#include <string>
#include <string_view>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client::tools {

    // JS-like readyState
    enum class ReadyState : int { CONNECTING = 0, OPEN = 1, CLOSED = 2 };

    // JS-like MessageEvent (simplified)
    struct MessageEvent {
        std::string type;        // "message" or custom from "event:"
        std::string data;        // joined data (LF between lines, trailing LF removed)
        std::string lastEventId; // from "id:"
        std::string origin;      // scheme://host[:port]
    };

    class EventSource {
    protected:
        using EventFn = std::function<void(const MessageEvent&)>;

    public:
        virtual ~EventSource();

        virtual EventSource* onMessage(EventFn fn) = 0;
        virtual EventSource* addEventListener(const std::string& key, EventFn fn) = 0;
        virtual EventSource* removeEventListeners(const std::string& type) = 0;
        virtual void close() = 0;
        virtual EventSource* onOpen(std::function<void()> onOpen) = 0;
        virtual EventSource* onError(std::function<void()> onError) = 0;
        virtual ReadyState readyState() const = 0;
        virtual const std::string& lastEventId() const = 0;
        virtual uint32_t retry() const = 0;
        virtual EventSource* retry(uint32_t retry) = 0;
    };

    template <typename Client>
    class EventSourceT
        : public EventSource
        , public std::enable_shared_from_this<EventSourceT<Client>> {
    public:
        using MasterRequest = typename Client::MasterRequest;
        using Request = typename Client::Request;
        using Response = typename Client::Response;
        using SocketConnection = typename Client::SocketConnection;

    private:
        struct SharedState {
            std::list<EventFn> onMessageListener;
            std::map<std::string, std::list<EventFn>> onEventListener;
            std::list<std::function<void()>> onOpenListener;
            std::list<std::function<void()>> onErrorListener;

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
            typename Client::Config* config;
        };

        static bool digits(std::string_view maybeDigitsAsString) {
            for (const char maybeDigit : maybeDigitsAsString) {
                if (std::isdigit(maybeDigit) == 0) {
                    return false;
                }
            }

            return !maybeDigitsAsString.empty();
        }

        static uint32_t parseU32(std::string_view uint32AsString) {
            uint64_t uint32AsUint64 = 0;

            for (const char digit : uint32AsString) {
                uint32AsUint64 = uint32AsUint64 * 10 + (static_cast<unsigned char>(digit) - '0');
                if (uint32AsUint64 > 0xFFFFFFFFULL) {
                    return 0xFFFFFFFFU;
                }
            }

            return static_cast<uint32_t>(uint32AsUint64);
        }

        static void deliverMessage(const std::shared_ptr<SharedState>& state,
                                   const std::string& evType,
                                   const std::string& payload,
                                   const std::string& evId) {
            if (auto it = state->onEventListener.find(evType); it != state->onEventListener.end()) {
                const MessageEvent e{evType, payload, evId, state->origin};
                for (const auto& onEventListener : it->second) {
                    onEventListener(e);
                }
            }

            if (evType == "message") {
                const MessageEvent e{evType, payload, evId, state->origin};

                for (const auto& onMessageListener : state->onMessageListener) {
                    onMessageListener(e);
                }
            }
        }

        static void dispatch(const std::shared_ptr<SharedState>& state) {
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

            deliverMessage(state, evType, payload, evId);
        }

        static void processLine(const std::shared_ptr<SharedState>& state, std::string_view line) {
            if (line.empty()) {
                dispatch(state);
                return;
            }
            if (line.front() == ':') {
                return;
            }

            std::string_view field;
            std::string_view value;

            if (auto p = line.find(':'); p != std::string_view::npos) {
                field = line.substr(0, p);
                value = line.substr(p + 1);
                if (!value.empty() && value.front() == ' ') {
                    value.remove_prefix(1);
                }
            } else {
                field = line;
            }

            if (field == "data") {
                state->data.append(value);
                state->data.push_back('\n');
            } else if (field == "event") {
                state->type = value;
            } else if (field == "id") {
                if (value.find('\0') == std::string_view::npos) {
                    state->idBuf = value;
                }
            } else if (field == "retry") {
                if (digits(value)) {
                    uint32_t const retry = parseU32(value);

                    state->retry = retry;
                    state->config->setReconnectTime(retry);
                    state->config->setRetryTimeout(retry);
                }
            }
            // else ignore
        }

        static void parse(const std::shared_ptr<SharedState>& state) {
            size_t start = 0;
            for (;;) {
                const size_t i = state->pending.find_first_of("\r\n", start);
                if (i == std::string::npos) {
                    if (start != 0U) {
                        state->pending.erase(0, start);
                    }
                    break;
                }

                if (state->pending[i] == '\r' && i + 1 >= state->pending.size()) {
                    if (start != 0U) {
                        state->pending.erase(0, start);
                    }
                    break;
                }

                const size_t eol = (state->pending[i] == '\r' && i + 1 < state->pending.size() && state->pending[i + 1] == '\n') ? 2 : 1;

                const std::string_view line(&state->pending[start], i - start);
                start = i + eol;
                processLine(state, line);
            }
        }

    public:
        EventSourceT(const EventSourceT&) = delete;
        EventSourceT& operator=(const EventSourceT&) = delete;
        EventSourceT(EventSourceT&&) noexcept = delete;
        EventSourceT& operator=(EventSourceT&&) noexcept = delete;

    protected:
        EventSourceT()
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

                const std::weak_ptr<EventSourceT> eventStreamWeak = this->weak_from_this();

                client = std::make_shared<Client>(
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnect";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
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

                        if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = nullptr;
                        }

                        if (state->config->getReconnect()) {
                            for (const auto& onError : state->onErrorListener) {
                                onError();
                            }
                            state->ready = ReadyState::CONNECTING;
                        } else {
                            state->ready = ReadyState::CLOSED;
                        }
                    },
                    [url = match[5].matched ? match[5].str() : "/",
                     state = this->state](const std::shared_ptr<MasterRequest>& masterRequest) {
                        const std::string connectionName = masterRequest->getSocketContext()->getSocketConnection()->getConnectionName();

                        LOG(DEBUG) << connectionName << " EventSource: OnRequestStart";

                        masterRequest->requestEventSource(
                            url,
                            [masterRequestWeak = std::weak_ptr<MasterRequest>(masterRequest), state, connectionName]() -> std::size_t {
                                std::size_t consumed = 0;

                                if (const std::shared_ptr<MasterRequest> masterRequest = masterRequestWeak.lock()) {
                                    char buf[16384];
                                    consumed = masterRequest->getSocketContext()->readFromPeer(buf, sizeof(buf));

                                    if (consumed > 0) {
                                        state->pending.append(buf, consumed);
                                        parse(state);
                                    }
                                } else {
                                    LOG(DEBUG) << connectionName << ": server-sent event: server disconnect";
                                }

                                return consumed;
                            },
                            [state, connectionName]() {
                                LOG(DEBUG) << connectionName << ": server-sent event stream start";

                                state->ready = ReadyState::OPEN;

                                for (const auto& onOpen : state->onOpenListener) {
                                    onOpen();
                                }

                                state->config->setReconnectTime(state->retry);
                                state->config->setRetryTimeout(state->retry);
                            },
                            [state, connectionName]() {
                                LOG(DEBUG) << connectionName << ": not an server-sent event endpoint: " << state->origin + state->path;
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
        EventSourceT* onMessage(EventFn fn) override {
            state->onMessageListener.push_back(std::move(fn));

            return this;
        }

        EventSourceT* addEventListener(const std::string& key, EventFn fn) override {
            state->onEventListener[key].push_back(std::move(fn));

            return this;
        }

        EventSourceT* removeEventListeners(const std::string& type) override {
            state->onEventListener.erase(type);

            return this;
        }

        void close() override {
            client->getConfig().setReconnect(false);

            if (socketConnection != nullptr) {
                socketConnection->shutdownRead();
            }
        }

        EventSourceT* onOpen(std::function<void()> onOpen) override {
            state->onOpenListener.push_back(std::move(onOpen));

            return this;
        }

        EventSourceT* onError(std::function<void()> onError) override {
            state->onErrorListener.push_back(std::move(onError));

            return this;
        }

        ReadyState readyState() const override {
            return state->ready;
        }

        const std::string& lastEventId() const override {
            return state->lastId;
        }

        uint32_t retry() const override {
            return state->retry;
        }

        EventSourceT* retry(uint32_t retry) override {
            state->retry = retry;
            state->config->setReconnectTime(retry);
            state->config->setRetryTimeout(retry);

            return this;
        }

        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<SharedState> state;
    };

} // namespace web::http::client::tools

#endif // WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
