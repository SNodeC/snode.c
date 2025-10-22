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

#ifndef WEB_HTTP_LEGACY_IN_EVENTSTREAM_H
#define WEB_HTTP_LEGACY_IN_EVENTSTREAM_H

#include "web/http/legacy/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <list>
#include <regex>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::in {

    using Client = web::http::legacy::in::Client;
    using MasterRequest = Client::MasterRequest;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketConnection = Client::SocketConnection;

    using DataFn = std::function<void(const std::string& data)>;
    using EventFn = std::function<void(const std::string& id, const std::string& data)>;

    struct SharedState {
        std::list<DataFn> dataCbList;
        std::map<std::string, std::list<EventFn>> eventCbMap;
    };

    class EventStream : public std::enable_shared_from_this<EventStream> {
    public:
        EventStream(const EventStream&) = delete;
        EventStream& operator=(const EventStream&) = delete;
        EventStream(EventStream&&) noexcept = delete;
        EventStream& operator=(EventStream&&) noexcept = delete;

        ~EventStream() = default;

    private:
        EventStream()
            : state(std::make_shared<SharedState>()) {
        }

        void init(const std::string& url) {
            static const std::regex re(R"(^((https?)://)?([^/:]+)(?::(\d+))?(/.*)?$)");

            std::smatch match;
            if (std::regex_match(url, match, re)) {
                VLOG(0) << "Full protocol: " << match[1];
                VLOG(0) << "     protocol: " << match[2];
                VLOG(0) << "         Host: " << match[3];
                VLOG(0) << "         Port: " << (match[4].matched ? match[4].str() : "80");
                VLOG(0) << "         Path: " << (match[5].matched ? match[5].str() : "/");

                const std::weak_ptr<EventStream> eventStreamWeak = weak_from_this();

                client = std::make_shared<Client>(
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        VLOG(1) << socketConnection->getConnectionName() << ": OnConnect";

                        VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventStream> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = socketConnection;
                        }
                    },
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        VLOG(1) << socketConnection->getConnectionName() << ": OnConnected";

                        VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
                    },
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";

                        VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventStream> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = nullptr;
                        }
                    },
                    [url = match[5].matched ? match[5].str() : "/",
                     state = this->state](const std::shared_ptr<MasterRequest>& masterRequest) {
                        VLOG(1) << masterRequest->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";

                        masterRequest->requestEventStream(
                            url, [masterRequest = std::weak_ptr<MasterRequest>(masterRequest), state]() -> std::size_t {
                                std::size_t consumed = 0;

                                if (!masterRequest.expired()) {
                                    char message[2048];
                                    consumed = masterRequest.lock()->getSocketContext()->readFromPeer(message, 2047);
                                    message[consumed] = 0;

                                    LOG(DEBUG) << "Message: " << message;

                                    for (const DataFn& dataCb : state->dataCbList) {
                                        dataCb(message);
                                    }

                                    const std::string event = "event";

                                    if (const auto it = state->eventCbMap.find(event); it != state->eventCbMap.end()) {
                                        for (const EventFn& eventCb : it->second) {
                                            eventCb("FakeId", "FakeData");
                                        }
                                    }
                                } else {
                                    VLOG(0) << "Server-sent event: server disconnect";
                                }

                                return consumed;
                            });
                    },
                    [](const std::shared_ptr<Request>& req) {
                        VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestEnd";
                    });

                client->getConfig().Remote::setHost(match[3].str());
                client->getConfig().Remote::setPort(match[4].matched ? static_cast<uint16_t>(std::stoi(match[4].str())) : 80);
                client->getConfig().setReconnect();
                client->getConfig().setRetry();

                client->connect([instanceName = client->getConfig().getInstanceName()](
                                    const core::socket::SocketAddress& socketAddress,
                                    const core::socket::State& state) { // example.com:81 simulate connnect timeout
                    switch (state) {
                        case core::socket::State::OK:
                            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                            break;
                        case core::socket::State::DISABLED:
                            VLOG(1) << instanceName << ": disabled";
                            break;
                        case core::socket::State::ERROR:
                            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                            break;
                        case core::socket::State::FATAL:
                            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                            break;
                    }
                });
            }
        }

    public:
        void onMessage(DataFn fn) {
            state->dataCbList.push_back(std::move(fn));
        }

        void addEventListener(const std::string& key, EventFn fn) {
            state->eventCbMap[key].push_back(std::move(fn));
        }

        void close() {
            client->getConfig().setReconnect(false);

            if (socketConnection != nullptr) {
                socketConnection->shutdownRead();
            }
        }

        std::shared_ptr<Client> client;
        SocketConnection* socketConnection = nullptr;
        std::shared_ptr<SharedState> state;

        friend inline std::shared_ptr<class EventStream> EventStream(const std::string& url);
    };

    inline std::shared_ptr<class EventStream> EventStream(const std::string& url) {
        std::shared_ptr<class EventStream> eventStream = std::shared_ptr<class EventStream>(new class EventStream());
        eventStream->init(url);
        return eventStream;
    }

} // namespace web::http::legacy::in

#endif // WEB_HTTP_LEGACY_IN_EVENTSTREAM_H
