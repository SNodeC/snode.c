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
#include <string>
#include <string_view>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client::tools {

    class EventSource {
    public:
        // JS-like readyState
        enum class ReadyState : int { CONNECTING = 0, OPEN = 1, CLOSED = 2 };

        // JS-like MessageEvent (simplified)
        struct MessageEvent {
            std::string type;        // "message" or custom from "event:"
            std::string data;        // joined data (LF between lines, trailing LF removed)
            std::string lastEventId; // from "id:"
            std::string origin;      // scheme://host[:port]
        };

    protected:
        EventSource();

    public:
        virtual ~EventSource();

        EventSource* onMessage(std::function<void(const MessageEvent&)> messageCallback);
        EventSource* addEventListener(const std::string& key, std::function<void(const MessageEvent&)> eventListener);
        EventSource* removeEventListeners(const std::string& type);
        EventSource* onOpen(std::function<void()> onOpen);
        EventSource* onError(std::function<void()> onError);
        EventSource::ReadyState readyState() const;
        const std::string& lastEventId() const;
        uint32_t retry() const;
        EventSource* retry(uint32_t retry);
        virtual void close() = 0;

    protected:
        struct SharedState {
            std::list<std::function<void(const MessageEvent&)>> onMessageListener;
            std::map<std::string, std::list<std::function<void(const MessageEvent&)>>> onEventListener;
            std::list<std::function<void()>> onOpenListener;
            std::list<std::function<void()>> onErrorListener;

            std::string pending;
            std::string data;
            std::string type;
            std::string idBuf;
            std::string lastId;
            uint32_t retry = 3000;

            std::string origin; // scheme://host[:port]
            std::string path;

            ReadyState ready = ReadyState::CONNECTING;
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
        using SocketAddress = typename SocketConnection::SocketAddress;
        using Config = typename Client::Config;

    private:
        struct SharedConfig {
            Config* config;
        };

        static bool digits(std::string_view maybeDigitsAsString) {
            for (const char maybeDigit : maybeDigitsAsString) {
                if (std::isdigit(static_cast<unsigned char>(maybeDigit)) == 0) {
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
            const MessageEvent e{evType, payload, evId, sharedState->origin};

            if (auto it = sharedState->onEventListener.find(evType); it != sharedState->onEventListener.end()) {
                for (const auto& onEventListener : it->second) {
                    onEventListener(e);
                }
            }

            if (evType == "message") {
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

        static bool processLine(const std::shared_ptr<SharedState>& sharedState,
                                const std::shared_ptr<SharedConfig>& sharedConfig,
                                std::string_view line) {
            bool success = true;

            if (!line.empty() && line.front() != ':') {
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
                    static constexpr size_t kMaxDataSize = 1 << 20;

                    success = sharedState->data.size() + value.size() + 1 <= kMaxDataSize; // Data length

                    if (success) {
                        sharedState->data.append(value);
                        sharedState->data.push_back('\n');
                    }
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
                        sharedConfig->config->setReconnectTime(retry / 1000.);
                        sharedConfig->config->setRetryTimeout(retry / 1000.);
                    }
                }
            }

            return success;
        }

        static bool parse(const std::shared_ptr<SharedState>& sharedState, const std::shared_ptr<SharedConfig>& sharedConfig) {
            static constexpr size_t kMaxLine = 256 * 1024;

            bool success = true;
            bool eol = true;
            do {
                const size_t i = sharedState->pending.find_first_of("\r\n");

                eol = i != std::string::npos && (sharedState->pending[i] != '\r' || i + 1 < sharedState->pending.size());

                if (eol && i <= kMaxLine) {
                    const size_t eolLength =
                        (sharedState->pending[i] == '\r' && i + 1 < sharedState->pending.size() && sharedState->pending[i + 1] == '\n') ? 2
                                                                                                                                        : 1;

                    const std::string_view line(sharedState->pending.data(), i);

                    if (!line.empty()) {
                        success = processLine(sharedState, sharedConfig, line);
                    } else {
                        dispatch(sharedState);
                    }

                    sharedState->pending.erase(0, i + eolLength);
                } else {
                    success = sharedState->pending.size() <= kMaxLine; // Line length
                }
            } while (eol && success);

            return success;
        }

    public:
        EventSourceT(const EventSourceT&) = delete;
        EventSourceT& operator=(const EventSourceT&) = delete;
        EventSourceT(EventSourceT&&) noexcept = delete;
        EventSourceT& operator=(EventSourceT&&) noexcept = delete;

    protected:
        EventSourceT()
            : sharedConfig(std::make_shared<SharedConfig>()) {
        }

        void init(const std::string& scheme, const SocketAddress& socketAddress, const std::string& path) {
            sharedState->path = path;
            sharedState->origin = scheme + "://" + socketAddress.toString(false);

            LOG(TRACE) << "Origin: " << sharedState->origin;
            LOG(TRACE) << "  Path: " << sharedState->path;

            const std::weak_ptr<EventSourceT> eventStreamWeak = this->weak_from_this();

            client = std::make_shared<Client>(
                [eventStreamWeak](SocketConnection* socketConnection) {
                    LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnect";

                    if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
                        eventStream->socketConnection = socketConnection;
                    }
                },
                [](SocketConnection* socketConnection) {
                    LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnConnected";
                },
                [eventStreamWeak, sharedState = this->sharedState, sharedConfig = this->sharedConfig](SocketConnection* socketConnection) {
                    LOG(DEBUG) << socketConnection->getConnectionName() << " EventSource: OnDisconnect";

                    if (const std::shared_ptr<EventSourceT> eventStream = eventStreamWeak.lock()) {
                        eventStream->socketConnection = nullptr;
                    }

                    if (sharedState->ready != ReadyState::CLOSED) {
                        if (auto it = sharedState->onEventListener.find("error"); it != sharedState->onEventListener.end()) {
                            EventSource::MessageEvent e{"error", "", sharedState->lastId, sharedState->origin};

                            for (auto& onError : it->second) {
                                onError(e);
                            }
                        }

                        for (const auto& onError : sharedState->onErrorListener) {
                            onError();
                        }

                        sharedState->ready = ReadyState::CONNECTING;
                        sharedState->data.clear();
                        sharedState->type.clear();
                        sharedState->idBuf.clear();
                        sharedState->pending.clear();

                        sharedConfig->config->setReconnectTime(sharedState->retry / 1000.);
                        sharedConfig->config->setRetryTimeout(sharedState->retry / 1000.);
                    }
                },
                [sharedState = this->sharedState, sharedConfig = this->sharedConfig](const std::shared_ptr<MasterRequest>& masterRequest) {
                    const std::string connectionName = masterRequest->getSocketContext()->getSocketConnection()->getConnectionName();

                    LOG(DEBUG) << connectionName << " EventSource: OnRequestStart";

                    if (!sharedState->lastId.empty()) {
                        masterRequest->set("Last-Event-ID", sharedState->lastId);
                    }

                    masterRequest->requestEventSource(
                        sharedState->path,
                        [masterRequestWeak = std::weak_ptr<MasterRequest>(masterRequest), sharedState, sharedConfig, connectionName]()
                            -> std::size_t {
                            std::size_t consumed = 0;

                            if (const std::shared_ptr<MasterRequest> masterRequest = masterRequestWeak.lock()) {
                                char buf[16384];
                                consumed = masterRequest->getSocketContext()->readFromPeer(buf, sizeof(buf));

                                sharedState->pending.append(buf, consumed);

                                if (!parse(sharedState, sharedConfig)) {
                                    masterRequest->getSocketContext()->shutdownWrite(true);
                                }
                            } else {
                                LOG(DEBUG) << connectionName << ": server-sent event: server disconnect";
                            }

                            return consumed;
                        },
                        [sharedState, sharedConfig, connectionName]() {
                            LOG(DEBUG) << connectionName << ": server-sent event stream start";

                            sharedState->ready = ReadyState::OPEN;

                            sharedConfig->config->setReconnectTime(sharedState->retry / 1000.);
                            sharedConfig->config->setRetryTimeout(sharedState->retry / 1000.);

                            if (auto it = sharedState->onEventListener.find("open"); it != sharedState->onEventListener.end()) {
                                EventSource::MessageEvent e{"open", "", sharedState->lastId, sharedState->origin};

                                for (auto& onOpen : it->second) {
                                    onOpen(e);
                                }
                            }

                            for (const auto& onOpen : sharedState->onOpenListener) {
                                onOpen();
                            }
                        },
                        [sharedState, connectionName]() {
                            LOG(DEBUG) << connectionName
                                       << ": not an server-sent event endpoint: " << sharedState->origin + sharedState->path;
                        });
                },
                [](const std::shared_ptr<Request>& req) {
                    LOG(DEBUG) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " EventSource: OnRequestEnd";
                });

            client->getConfig().Remote::setSocketAddress(socketAddress);
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

    public:
        void close() override {
            sharedState->ready = ReadyState::CLOSED;

            sharedConfig->config->setReconnect(false);
            sharedConfig->config->setRetry(false);

            if (socketConnection != nullptr) {
                socketConnection->shutdownWrite(true);
            }
        }

    private:
        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<SharedConfig> sharedConfig;
    };

} // namespace web::http::client::tools

#endif // WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
