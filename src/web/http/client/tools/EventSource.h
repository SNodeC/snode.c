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
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <regex>
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

        EventSource();

    public:
        virtual ~EventSource();

        EventSource* onMessage(EventFn fn);
        EventSource* addEventListener(const std::string& key, EventFn fn);
        EventSource* removeEventListeners(const std::string& type);
        EventSource* onOpen(std::function<void()> onOpen);
        EventSource* onError(std::function<void()> onError);
        ReadyState readyState() const;
        const std::string& lastEventId() const;
        uint32_t retry() const;

        virtual void close() = 0;
        virtual EventSource* retry(uint32_t retry) = 0;

    protected:
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
        };

        std::shared_ptr<SharedState> sharedState;
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
        struct Config {
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

        static void deliverMessage(const std::shared_ptr<SharedState>& sharedState,
                                   const std::string& evType,
                                   const std::string& payload,
                                   const std::string& evId) {
            if (auto it = sharedState->onEventListener.find(evType); it != sharedState->onEventListener.end()) {
                const MessageEvent e{evType, payload, evId, sharedState->origin};
                for (const auto& onEventListener : it->second) {
                    onEventListener(e);
                }
            }

            if (evType == "message") {
                const MessageEvent e{evType, payload, evId, sharedState->origin};

                for (const auto& onMessageListener : sharedState->onMessageListener) {
                    onMessageListener(e);
                }
            }
        }

        static void dispatch(const std::shared_ptr<SharedState>& sharedState) {
            sharedState->lastId = sharedState->idBuf;
            if (sharedState->data.empty()) {
                sharedState->type.clear();
                return;
            }
            if (!sharedState->data.empty() && sharedState->data.back() == '\n') {
                sharedState->data.pop_back();
            }

            const std::string evType = sharedState->type.empty() ? "message" : std::exchange(sharedState->type, std::string{});
            const std::string payload = std::exchange(sharedState->data, std::string{});
            const std::string evId = sharedState->lastId;

            deliverMessage(sharedState, evType, payload, evId);
        }

        static void
        processLine(const std::shared_ptr<SharedState>& sharedState, const std::shared_ptr<Config>& sharedConfig, std::string_view line) {
            if (line.empty()) {
                dispatch(sharedState);
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
                sharedState->data.append(value);
                sharedState->data.push_back('\n');
            } else if (field == "event") {
                sharedState->type = value;
            } else if (field == "id") {
                if (value.find('\0') == std::string_view::npos) {
                    sharedState->idBuf = value;
                }
            } else if (field == "retry") {
                if (digits(value)) {
                    uint32_t const retry = parseU32(value);

                    sharedState->retry = retry;
                    sharedConfig->config->setReconnectTime(retry);
                    sharedConfig->config->setRetryTimeout(retry);
                }
            }
            // else ignore
        }

        static void parse(const std::shared_ptr<SharedState>& sharedState, const std::shared_ptr<Config>& sharedConfig) {
            size_t start = 0;
            for (;;) {
                const size_t i = sharedState->pending.find_first_of("\r\n", start);
                if (i == std::string::npos) {
                    if (start != 0U) {
                        sharedState->pending.erase(0, start);
                    }
                    break;
                }

                if (sharedState->pending[i] == '\r' && i + 1 >= sharedState->pending.size()) {
                    if (start != 0U) {
                        sharedState->pending.erase(0, start);
                    }
                    break;
                }

                const size_t eol =
                    (sharedState->pending[i] == '\r' && i + 1 < sharedState->pending.size() && sharedState->pending[i + 1] == '\n') ? 2 : 1;

                const std::string_view line(&sharedState->pending[start], i - start);
                start = i + eol;
                processLine(sharedState, sharedConfig, line);
            }
        }

    public:
        EventSourceT(const EventSourceT&) = delete;
        EventSourceT& operator=(const EventSourceT&) = delete;
        EventSourceT(EventSourceT&&) noexcept = delete;
        EventSourceT& operator=(EventSourceT&&) noexcept = delete;

    protected:
        EventSourceT()
            : sharedConfig(std::make_shared<Config>()) {
        }

        void init(const std::string& url) {
            static const std::regex re(R"(^((https?)://)?([^/:]+)(?::(\d+))?(/.*)?$)");

            std::smatch match;
            if (std::regex_match(url, match, re)) {
                // capture URL parts for origin/ports
                sharedState->scheme = (match[2].matched ? match[2].str() : "http");
                sharedState->host = match[3].str();
                const bool isHttps = (sharedState->scheme == "https");
                sharedState->port =
                    match[4].matched ? static_cast<uint16_t>(std::stoi(match[4].str())) : static_cast<uint16_t>(isHttps ? 443 : 80);
                sharedState->origin = sharedState->scheme + std::string("://") + sharedState->host +
                                      (((isHttps && sharedState->port == 443) || (!isHttps && sharedState->port == 80))
                                           ? ""
                                           : (":" + std::to_string(sharedState->port)));

                sharedState->path = (match[5].matched ? match[5].str() : "/");

                LOG(TRACE) << "Full protocol: " << match[1];
                LOG(TRACE) << "     protocol: " << sharedState->scheme;
                LOG(TRACE) << "         Host: " << sharedState->host;
                LOG(TRACE) << "         Port: " << sharedState->port;
                LOG(TRACE) << "         Path: " << sharedState->path;
                LOG(TRACE) << "       Origin: " << sharedState->origin;

                const std::weak_ptr<EventSourceT> eventStreamWeak = this->weak_from_this();

                client = std::make_shared<Client>(
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnect";

                        if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = socketConnection;
                        }
                    },
                    [state = this->sharedState](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnected";
                    },
                    [eventStreamWeak, sharedState = this->sharedState, sharedConfig = this->sharedConfig](
                        SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnDisconnect";

                        if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = nullptr;
                        }

                        if (sharedConfig->config->getReconnect()) {
                            for (const auto& onError : sharedState->onErrorListener) {
                                onError();
                            }
                            sharedState->ready = ReadyState::CONNECTING;
                        } else {
                            sharedState->ready = ReadyState::CLOSED;
                        }
                    },
                    [url = match[5].matched ? match[5].str() : "/", sharedState = this->sharedState, sharedConfig = this->sharedConfig](
                        const std::shared_ptr<MasterRequest>& masterRequest) {
                        const std::string connectionName = masterRequest->getSocketContext()->getSocketConnection()->getConnectionName();

                        LOG(DEBUG) << connectionName << " EventSource: OnRequestStart";

                        masterRequest->requestEventSource(
                            url,
                            [masterRequestWeak = std::weak_ptr<MasterRequest>(masterRequest), sharedState, sharedConfig, connectionName]()
                                -> std::size_t {
                                std::size_t consumed = 0;

                                if (const std::shared_ptr<MasterRequest> masterRequest = masterRequestWeak.lock()) {
                                    char buf[16384];
                                    consumed = masterRequest->getSocketContext()->readFromPeer(buf, sizeof(buf));

                                    if (consumed > 0) {
                                        sharedState->pending.append(buf, consumed);
                                        parse(sharedState, sharedConfig);
                                    }
                                } else {
                                    LOG(DEBUG) << connectionName << ": server-sent event: server disconnect";
                                }

                                return consumed;
                            },
                            [sharedState, sharedConfig, connectionName]() {
                                LOG(DEBUG) << connectionName << ": server-sent event stream start";

                                sharedState->ready = ReadyState::OPEN;

                                for (const auto& onOpen : sharedState->onOpenListener) {
                                    onOpen();
                                }

                                sharedConfig->config->setReconnectTime(sharedState->retry);
                                sharedConfig->config->setRetryTimeout(sharedState->retry);
                            },
                            [sharedState, connectionName]() {
                                LOG(DEBUG) << connectionName
                                           << ": not an server-sent event endpoint: " << sharedState->origin + sharedState->path;
                            });
                    },
                    [](const std::shared_ptr<Request>& req) {
                        LOG(DEBUG) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " EventSource: OnRequestEnd";
                    });

                client->getConfig().Remote::setHost(sharedState->host);
                client->getConfig().Remote::setPort(sharedState->port);
                client->getConfig().setReconnect();
                client->getConfig().setRetry();
                client->getConfig().setRetryBase(1);

                sharedConfig->config = &client->getConfig();

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
        void close() override {
            sharedConfig->config->setReconnect(false);

            if (socketConnection != nullptr) {
                socketConnection->shutdownRead();
            }
        }

        EventSource* retry(uint32_t retry) override {
            sharedState->retry = retry;
            sharedConfig->config->setReconnectTime(retry);
            sharedConfig->config->setRetryTimeout(retry);

            return this;
        }

        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<Config> sharedConfig;
    };

} // namespace web::http::client::tools

#endif // WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
