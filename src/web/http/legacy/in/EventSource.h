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
#include <optional>
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

    class EventStream : public std::enable_shared_from_this<EventStream> {
    private:
        using DataFn = std::function<void(const std::string& data)>;
        using EventFn = std::function<void(const std::string& id, const std::string& data)>;

        struct SharedState {
            std::list<DataFn> dataCbList;
            std::map<std::string, std::list<EventFn>> eventCbMap;

            std::string pending_;
            std::string data_;
            std::string type_;
            std::string idBuf_;
            std::string lastId_;
            std::optional<uint32_t> retry_;
        };

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
                LOG(TRACE) << "Full protocol: " << match[1];
                LOG(TRACE) << "     protocol: " << match[2];
                LOG(TRACE) << "         Host: " << match[3];
                LOG(TRACE) << "         Port: " << (match[4].matched ? match[4].str() : "80");
                LOG(TRACE) << "         Path: " << (match[5].matched ? match[5].str() : "/");

                const std::weak_ptr<EventStream> eventStreamWeak = weak_from_this();

                client = std::make_shared<Client>(
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventStream: OnConnect";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventStream> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = socketConnection;
                        }
                    },
                    [](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventStream: OnConnected";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
                    },
                    [eventStreamWeak](SocketConnection* socketConnection) {
                        LOG(DEBUG) << socketConnection->getConnectionName() << " EventStream: OnDisconnect";

                        LOG(DEBUG) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                        LOG(DEBUG) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

                        if (const std::shared_ptr<EventStream> eventStream = eventStreamWeak.lock()) {
                            eventStream->socketConnection = nullptr;
                        }
                    },
                    [url = match[5].matched ? match[5].str() : "/",
                     state = this->state](const std::shared_ptr<MasterRequest>& masterRequest) {
                        LOG(DEBUG) << masterRequest->getSocketContext()->getSocketConnection()->getConnectionName()
                                   << " EventStream: OnRequestStart";

                        masterRequest->requestEventStream(
                            url, [masterRequest = std::weak_ptr<MasterRequest>(masterRequest), state]() -> std::size_t {
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

                                auto dispatch = [state]() mutable {
                                    state->lastId_ = state->idBuf_;
                                    if (state->data_.empty()) {
                                        state->type_.clear();
                                        return;
                                    }
                                    if (!state->data_.empty() && state->data_.back() == '\n') {
                                        state->data_.pop_back();
                                    }
                                    /*
                                    if (cb_)
                                        cb_({type_.empty() ? "message" : type_, std::move(data_), lastId_});
                                    */

                                    VLOG(0) << "Message type: " << (state->type_.empty() ? "message" : state->type_);
                                    VLOG(0) << "Data: " << state->data_;
                                    VLOG(0) << "LastId: " << state->lastId_;

                                    state->type_.clear();
                                    state->data_.clear();
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
                                        state->data_.append(v);
                                        state->data_.push_back('\n');
                                    } else if (f == "event") {
                                        state->type_ = v;
                                    } else if (f == "id") {
                                        if (v.find('\0') == std::string_view::npos) {
                                            state->idBuf_ = v;
                                        }
                                    } else if (f == "retry") {
                                        if (digits(v)) {
                                            state->retry_ = parseU32(v);
                                        }
                                    }
                                    // else ignore
                                };

                                auto parse = [state, &processLine]() {
                                    size_t start = 0;
                                    for (;;) {
                                        const size_t i = state->pending_.find_first_of("\r\n", start);
                                        if (i == std::string::npos) {
                                            if (start) {
                                                state->pending_.erase(0, start);
                                            }
                                            break;
                                        }
                                        const size_t eol =
                                            (state->pending_[i] == '\r' && i + 1 < state->pending_.size() && state->pending_[i + 1] == '\n')
                                                ? 2
                                                : 1;
                                        const std::string_view line(&state->pending_[start], i - start);
                                        start = i + eol;
                                        processLine(line);
                                    }
                                };

                                std::size_t consumed = 0;

                                if (!masterRequest.expired()) {
                                    char buf[16384];
                                    consumed = masterRequest.lock()->getSocketContext()->readFromPeer(buf, sizeof(buf));

                                    if (consumed > 0) {
                                        state->pending_.append(buf, consumed);
                                        parse();
                                    }
                                    /*
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
                                    */
                                } else {
                                    LOG(DEBUG) << "Server-sent event: server disconnect";
                                }

                                return consumed;
                            });
                    },
                    [](const std::shared_ptr<Request>& req) {
                        LOG(DEBUG) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " EventStream: OnRequestEnd";
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

        friend inline std::shared_ptr<class EventStream> EventSource(const std::string& url);
    };

    inline std::shared_ptr<class EventStream> EventSource(const std::string& url) {
        std::shared_ptr<class EventStream> eventSource = std::shared_ptr<class EventStream>(new class EventStream());
        eventSource->init(url);
        return eventSource;
    }

} // namespace web::http::legacy::in

#endif // WEB_HTTP_LEGACY_IN_EVENTSOURCE_H
