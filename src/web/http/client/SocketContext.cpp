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

#include "web/http/client/SocketContext.h" // IWYU pragma: export

#include "core/EventReceiver.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/CookieOptions.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// #define TRUE 1
// #define FALSE 0

namespace web::http::client {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onHttpConnected,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onHttpDisconnected,
                                 const std::string& hostHeader,
                                 bool pipelinedRequests)
        : Super(socketConnection)
        , onHttpConnected(onHttpConnected)
        , onHttpDisconnected(onHttpDisconnected)
        , pipelinedRequests(pipelinedRequests)
        , masterRequest(std::make_shared<Request>(this, hostHeader))
        , parser(
              this,
              [this]() {
                  responseStarted();
              },
              [this](web::http::client::Response&& response) {
                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response parsed: HTTP/" << response.httpMajor << "."
                            << response.httpMinor << " " << response.statusCode << " " << response.reason;

                  deliverResponse(std::move(response));
              },
              [this](int status, const std::string& reason) {
                  deliverResponseParseError(status, reason);
              }) {
        masterRequest->setMasterRequest(masterRequest);
    }

    SocketContext::~SocketContext() {
        if (!deliveredRequests.empty()) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Responses missed";
            for (const std::shared_ptr<Request>& request : deliveredRequests) {
                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;
            }
        }

        if (!pendingRequests.empty()) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Requests ignored";
            for (const std::shared_ptr<Request>& request : pendingRequests) {
                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;
            }
        }
    }

    void SocketContext::requestPrepared(const std::shared_ptr<Request>& request) {
        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        if ((flags == Flags::NONE || (flags & Flags::HTTP11) == Flags::HTTP11 || (flags & Flags::KEEPALIVE) == Flags::KEEPALIVE) &&
            (flags & Flags::CLOSE) != Flags::CLOSE) {
            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request accepted: " << requestLine;

            flags = (flags & ~Flags::HTTP11) | ((request->httpMajor == 1 && request->httpMinor == 1) ? Flags::HTTP11 : Flags::NONE);
            flags = (flags & ~Flags::HTTP10) | ((request->httpMajor == 1 && request->httpMinor == 0) ? Flags::HTTP10 : Flags::NONE);
            flags = (flags & ~Flags::KEEPALIVE) |
                    (web::http::ciContains(request->header("Connection"), "keep-alive") ? Flags::KEEPALIVE : Flags::NONE);
            flags = (flags & ~Flags::CLOSE) | (web::http::ciContains(request->header("Connection"), "close") ? Flags::CLOSE : Flags::NONE);

            pendingRequests.push_back(request);

            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request queued: " << requestLine;

            if (!reqInProgress) {
                initiateRequest();
            }
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request rejected: " << requestLine;
        }
    }

    void SocketContext::initiateRequest() {
        const std::shared_ptr<Request> request = pendingRequests.front();

        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request start: " << requestLine;

        if (request->initiate(request)) {
            reqInProgress = true;
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request delivering failed: " << requestLine;

            core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                if (!masterRequest.expired() && !reqInProgress) {
                    if (!pendingRequests.empty()) {
                        const std::shared_ptr<Request> request = pendingRequests.front();

                        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request dequeued 1: " << request->method << " "
                                   << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;

                        initiateRequest();
                    }
                }
            });
        }
    }

    void SocketContext::requestDelivered(bool success) {
        const std::shared_ptr<Request> currentRequest = pendingRequests.front();
        pendingRequests.pop_front();

        const std::string requestLine = std::string(currentRequest->method)
                                            .append(" ")
                                            .append(currentRequest->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(currentRequest->httpMajor))
                                            .append(".")
                                            .append(std::to_string(currentRequest->httpMinor));

        if (success) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request delivered: " << requestLine;

            deliveredRequests.push_back(currentRequest);

            if (pipelinedRequests) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        if (!pendingRequests.empty()) {
                            const std::shared_ptr<Request> request = pendingRequests.front();

                            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request dequeued 2: " << request->method
                                       << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;

                            initiateRequest();
                        }
                    }
                });
            }
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request deliver failed: " << requestLine;

            reqInProgress = false;
            shutdownWrite();
        }
    }

    void SocketContext::responseStarted() {
        if (!deliveredRequests.empty()) {
            const std::string requestLine = std::string(deliveredRequests.front()->method)
                                                .append(" ")
                                                .append(deliveredRequests.front()->url)
                                                .append(" HTTP/")
                                                .append(std::to_string(deliveredRequests.front()->httpMajor))
                                                .append(".")
                                                .append(std::to_string(deliveredRequests.front()->httpMinor));

            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response start: " << requestLine;
        } else {
            LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP: Response without delivered request";

            shutdownWrite(true);
        }
    }

    void SocketContext::deliverResponse(Response&& response) {
        const std::shared_ptr<Request> request = std::move(deliveredRequests.front());
        deliveredRequests.pop_front();

        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response received: " << requestLine;

        const bool httpClose =
            response.connectionState == ConnectionState::Close ||
            (response.connectionState == ConnectionState::Default &&
             ((response.httpMajor == 0 && response.httpMinor == 9) || (response.httpMajor == 1 && response.httpMinor == 0)));

        request->deliverResponse(request, std::make_shared<Response>(std::move(response)));

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response completed: " << requestLine;

        responseDelivered(httpClose);
    }

    void SocketContext::deliverResponseParseError(int status, const std::string& reason) {
        const std::shared_ptr<Request> request = std::move(deliveredRequests.front());
        deliveredRequests.pop_front();

        LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Response parse error: " << reason << " (" << status << ") "
                     << std::string(request->method)
                            .append(" ")
                            .append(request->url)
                            .append(" HTTP/")
                            .append(std::to_string(request->httpMajor))
                            .append(".")
                            .append(std::to_string(request->httpMinor));

        request->deliverResponseParseError(request, reason);

        shutdownWrite(true);
    }

    void SocketContext::responseDelivered(bool httpClose) {
        if (httpClose) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";
        } else {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";

            if (!pipelinedRequests) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        if (!pendingRequests.empty()) {
                            const std::shared_ptr<Request> request = pendingRequests.front();

                            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Initiating request: " << request->method
                                       << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;

                            initiateRequest();
                        }
                    }
                });
            }
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";

        onHttpConnected(masterRequest);
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        if (!deliveredRequests.empty()) {
            consumed = parser.parse();
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        if (!deliveredRequests.empty()) {
            const std::shared_ptr<Request> request = deliveredRequests.front();
            if (request->httpMajor == 1 && request->httpMinor == 0) {
                Response response;
                response.httpVersion = "HTTP/1.1";
                response.httpMajor = 1;
                response.httpMinor = 1;
                response.statusCode = "0";
                response.reason = "Connection loss";

                deliverResponse(std::move(response));
            }
        }

        onHttpDisconnected(masterRequest);

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received disconnect";
    }

    bool SocketContext::onSignal([[maybe_unused]] int signum) {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received signal " << signum;

        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::client
