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

#ifndef WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
#define WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H

#include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/State.h"
#include "log/SemanticLogger.h"
#include "web/http/CiStringMap.h"
#include "web/http/client/SemanticLog.h"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>

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
            bool firstLine = true;
            bool skipLf = false;
            std::string data;
            std::string type;
            std::string idBuf;
            std::string lastId;
            uint32_t retry = 3000;

            std::string origin; // scheme://host[:port]
            std::string path;

            ReadyState ready = ReadyState::CONNECTING;
        };

        static bool feed(const std::shared_ptr<SharedState>& state, std::string_view chunk);

    private:
        static bool digits(std::string_view value);
        static uint32_t parseU32(std::string_view value);
        static void
        deliverMessage(const std::shared_ptr<SharedState>& state, const std::string& type, const std::string& data, const std::string& id);
        static void dispatch(const std::shared_ptr<SharedState>& state);
        static bool processLine(const std::shared_ptr<SharedState>& state, std::string_view line);
        static bool parse(const std::shared_ptr<SharedState>& state);

    protected:
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
            Config* config = nullptr;
            bool started = false;
            bool stopped = false;
            bool reconnectScheduled = false;
        };

    public:
        ~EventSourceT() override {
            close();
        }

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

            web::http::client::semantic::httpClientLog().trace() << "Origin: " << sharedState->origin;
            web::http::client::semantic::httpClientLog().trace() << "  Path: " << sharedState->path;

            const std::weak_ptr<EventSourceT> eventSourceWeak = this->weak_from_this();

            client = std::make_shared<Client>(
                [eventSourceWeak](SocketConnection* socketConnection) {
                    web::http::client::semantic::httpClientLog(*socketConnection).debug() << "OnConnect";

                    if (const std::shared_ptr<EventSourceT> eventStream = eventSourceWeak.lock()) {
                        eventStream->socketConnection = socketConnection;
                    }
                },
                [](SocketConnection* socketConnection) {
                    web::http::client::semantic::httpClientLog(*socketConnection).debug() << "OnConnected";
                },
                [eventSourceWeak, sharedState = this->sharedState, sharedConfig = this->sharedConfig](SocketConnection* socketConnection) {
                    web::http::client::semantic::httpClientLog(*socketConnection).debug() << "OnDisconnect";

                    if (const std::shared_ptr<EventSourceT> eventSource = eventSourceWeak.lock()) {
                        eventSource->socketConnection = nullptr;
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

                        if (sharedState->ready == ReadyState::CLOSED) {
                            return;
                        }

                        sharedState->ready = ReadyState::CONNECTING;
                        sharedState->data.clear();
                        sharedState->type.clear();
                        sharedState->idBuf.clear();
                        sharedState->pending.clear();
                        sharedState->firstLine = true;
                        sharedState->skipLf = false;

                        sharedConfig->config->setReconnectTime(sharedState->retry / 1000.);
                        sharedConfig->config->setRetryTimeout(sharedState->retry / 1000.);
                        if (!sharedConfig->reconnectScheduled) {
                            sharedConfig->reconnectScheduled = true;
                            web::http::client::semantic::httpClientEventSourceLog().debug()
                                << "event source reconnect scheduled: " << sharedState->origin + sharedState->path;
                        }
                    }
                },
                [eventSourceWeak, sharedState = this->sharedState, sharedConfig = this->sharedConfig](
                    const std::shared_ptr<MasterRequest>& masterRequest) {
                    auto* socketConnection = masterRequest->getSocketContext()->getSocketConnection();

                    web::http::client::semantic::httpClientLog(*socketConnection).debug() << "OnRequestStart";

                    if (sharedConfig->reconnectScheduled) {
                        sharedConfig->reconnectScheduled = false;
                        web::http::client::semantic::httpClientEventSourceLog().debug()
                            << "event source reconnect dispatched: " << sharedState->origin + sharedState->path;
                    }

                    if (!sharedState->lastId.empty()) {
                        masterRequest->set("Last-Event-ID", sharedState->lastId);
                    }

                    masterRequest->url = sharedState->path;
                    masterRequest->set("Accept", "text/event-stream");
                    masterRequest->set("Connection", "keep-alive");
                    const bool sent = masterRequest->stream(
                        {},
                        [sharedState](const std::shared_ptr<Response>& response) {
                            if (response->statusCode == "200" &&
                                web::http::ciContains(response->headers["Content-Type"], "text/event-stream")) {
                                sharedState->ready = ReadyState::OPEN;
                                web::http::client::semantic::httpClientEventSourceLog().info()
                                    << "event stream established: " << sharedState->origin + sharedState->path;
                                for (const auto& callback : sharedState->onOpenListener) {
                                    callback();
                                }
                                if (auto it = sharedState->onEventListener.find("open"); it != sharedState->onEventListener.end()) {
                                    const MessageEvent event{"open", "", sharedState->lastId, sharedState->origin};
                                    for (const auto& callback : it->second) {
                                        callback(event);
                                    }
                                }
                            } else {
                                for (const auto& callback : sharedState->onErrorListener) {
                                    callback();
                                }
                            }
                        },
                        [sharedState, sharedConfig, eventSourceWeak](std::string_view chunk) {
                            if (!feed(sharedState, chunk)) {
                                if (const auto source = eventSourceWeak.lock(); source && source->socketConnection != nullptr) {
                                    source->socketConnection->close();
                                }
                            }
                            sharedConfig->config->setReconnectTime(sharedState->retry / 1000.);
                            sharedConfig->config->setRetryTimeout(sharedState->retry / 1000.);
                        },
                        []() {
                        },
                        [sharedState](const std::string&) {
                            for (const auto& callback : sharedState->onErrorListener) {
                                callback();
                            }
                        });
                    if (!sent) {
                        if (const auto source = eventSourceWeak.lock(); source && source->socketConnection != nullptr) {
                            source->socketConnection->close();
                        }
                    }
                },
                [eventSourceWeak](const std::shared_ptr<Request>& req) {
                    if (const std::shared_ptr<EventSourceT> eventSource = eventSourceWeak.lock();
                        eventSource && eventSource->socketConnection != nullptr) {
                        web::http::client::semantic::httpClientLog(*eventSource->socketConnection).debug() << "OnRequestEnd";
                    } else {
                        web::http::client::semantic::httpClientLog().debug() << req->getConnectionName() << ": OnRequestEnd";
                    }
                });

            client->getConfig()->Remote::setSocketAddress(socketAddress);
            client->getConfig()->setReconnect();
            client->getConfig()->setRetry();
            client->getConfig()->setRetryBase(1);
            client->getConfig()->setInstanceName("EventSource");

            sharedConfig->config = client->getConfig();

            sharedConfig->started = true;
            web::http::client::semantic::httpClientEventSourceLog().info()
                << "event source started: " << sharedState->origin + sharedState->path;

            client->connect([instanceName = client->getConfig()->getInstanceName()](
                                const core::socket::SocketAddress& socketAddress,
                                const core::socket::State& state) { // example.com:81 simulate connnect timeout
                switch (state) {
                    case core::socket::State::OK:
                        web::http::client::semantic::httpClientLog().debug()
                            << instanceName << ": connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        web::http::client::semantic::httpClientLog().debug() << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        web::http::client::semantic::httpClientLog().debug()
                            << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        web::http::client::semantic::httpClientLog().debug()
                            << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            });
        }

    public:
        void close() override {
            if (sharedConfig->stopped) {
                return;
            }

            sharedState->ready = ReadyState::CLOSED;

            if (sharedConfig->reconnectScheduled) {
                sharedConfig->reconnectScheduled = false;
                web::http::client::semantic::httpClientEventSourceLog().debug()
                    << "event source reconnect cancelled: " << sharedState->origin + sharedState->path;
            }

            if (sharedConfig->config != nullptr) {
                sharedConfig->config->setReconnect(false);
                sharedConfig->config->setRetry(false);
            }

            if (socketConnection != nullptr) {
                socketConnection->shutdownWrite();
            }

            sharedConfig->stopped = true;
            if (sharedConfig->started) {
                web::http::client::semantic::httpClientEventSourceLog().info()
                    << "event source stopped: " << sharedState->origin + sharedState->path;
            }
        }

    private:
        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<SharedConfig> sharedConfig;
    };

} // namespace web::http::client::tools

#endif // WEB_HTTP_CLIENT_TOOLS_EVENTSOURCE_H
