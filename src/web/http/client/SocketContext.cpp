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
#include "utils/Timeval.h"
#include "web/http/http_utils.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// #define TRUE 1
// #define FALSE 0

namespace web::http::client {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                 const std::function<void(const std::shared_ptr<MasterRequest>&)>& onHttpConnected,
                                 const std::function<void(const std::shared_ptr<MasterRequest>&)>& onHttpDisconnected,
                                 const std::string& hostHeader,
                                 bool pipelinedRequests)
        : Super(socketConnection)
        , onHttpConnected(onHttpConnected)
        , onHttpDisconnected(onHttpDisconnected)
        , pipelinedRequests(pipelinedRequests)
        , masterRequest(std::make_shared<MasterRequest>(this, hostHeader))
        , parser(
              this,
              [this]() {
                  responseStarted();
              },
              [this](web::http::client::Response& response) {
                  deliverResponse(std::make_shared<Response>(std::move(response)));
              },
              [this](int status, const std::string& reason) {
                  deliverResponseParseError(status, reason);
              }) {
        masterRequest->setMasterRequest(masterRequest);
    }

    SocketContext::~SocketContext() {
        if (!deliveredRequests.empty()) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Responses missed";
            for (const std::shared_ptr<MasterRequest>& request : deliveredRequests) {
                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;
            }
        }

        if (!pendingRequests.empty()) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Requests ignored";
            for (const std::shared_ptr<MasterRequest>& request : pendingRequests) {
                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;
            }
        }
    }

    void SocketContext::requestPrepared(const std::shared_ptr<MasterRequest>& request) {
        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        if ((flags == Flags::NONE || (flags & Flags::HTTP11) == Flags::HTTP11 || (flags & Flags::KEEPALIVE) == Flags::KEEPALIVE) &&
            (flags & Flags::CLOSE) != Flags::CLOSE) {
            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count
                      << ") accepted: " << requestLine;
            flags = (flags & Flags::HTTP11) | ((request->httpMajor == 1 && request->httpMinor == 1) ? Flags::HTTP11 : Flags::NONE);
            flags = (flags & Flags::HTTP10) | ((request->httpMajor == 1 && request->httpMinor == 0) ? Flags::HTTP10 : Flags::NONE);
            flags = (flags & Flags::KEEPALIVE) |
                    (web::http::ciContains(request->header("Connection"), "keep-alive") ? Flags::KEEPALIVE : Flags::NONE);
            flags = (flags & Flags::CLOSE) | (web::http::ciContains(request->header("Connection"), "close") ? Flags::CLOSE : Flags::NONE);

            pendingRequests.push_back(request);

            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") queued: " << requestLine
                       << " - QueueSize = " << pendingRequests.size() << " - Flags: " << flags << " - "
                       << web::http::ciContains(request->header("Connection"), "close");

            if (pendingRequests.size() == 1) {
                initiateRequest();
            }
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count
                         << ") rejected: " << requestLine;

            LOG(WARNING) << httputils::toString(request->method,
                                                request->url,
                                                "HTTP/" + std::to_string(request->httpMajor) + "." + std::to_string(request->httpMinor),
                                                request->getQueries(),
                                                request->getHeaders(),
                                                request->getTrailer(),
                                                request->getCookies(),
                                                {});
        }
    }

    void SocketContext::initiateRequest() {
        if (!pendingRequests.empty()) {
            const std::shared_ptr<MasterRequest>& request = pendingRequests.front();

            const std::string requestLine = std::string(request->method)
                                                .append(" ")
                                                .append(request->url)
                                                .append(" HTTP/")
                                                .append(std::to_string(request->httpMajor))
                                                .append(".")
                                                .append(std::to_string(request->httpMinor));

            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") start: " << requestLine;

            if (!request->initiate(request)) {
                LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count
                             << ") delivering failed: " << requestLine;

                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    pendingRequests.pop_front();
                    if (!masterRequest.expired() && !pendingRequests.empty()) {
                        const std::shared_ptr<Request>& request = pendingRequests.front();

                        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count
                                   << ") dequeued: " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "."
                                   << request->httpMinor;

                        initiateRequest();
                    }
                });
            }
        }
    }

    void SocketContext::requestDelivered(bool success) {
        const std::shared_ptr<MasterRequest> currentRequest = std::move(pendingRequests.front());
        pendingRequests.pop_front();

        const std::string requestLine = std::string(currentRequest->method)
                                            .append(" ")
                                            .append(currentRequest->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(currentRequest->httpMajor))
                                            .append(".")
                                            .append(std::to_string(currentRequest->httpMinor));

        if (success) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << currentRequest->count
                       << ") delivered: " << requestLine << " " << pendingRequests.size();

            deliveredRequests.push_back(currentRequest);

            if (pipelinedRequests && !pendingRequests.empty()) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        const std::shared_ptr<Request>& request = pendingRequests.front();

                        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count
                                   << ") dequeued: " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "."
                                   << request->httpMinor;

                        initiateRequest();
                    }
                });
            }
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << currentRequest->count
                         << ") deliver failed: " << requestLine;

            shutdownWrite();
        }
    }

    void SocketContext::responseStarted() {
        if (deliveredRequests.empty()) {
            LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP: Response without delivered request";

            shutdownWrite(true);
        }
    }

    void SocketContext::deliverResponse(const std::shared_ptr<Response>& response) {
        const std::shared_ptr<MasterRequest> request = std::move(deliveredRequests.front());
        deliveredRequests.pop_front();

        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response received for request (" << request->count
                  << "): " << requestLine;

        LOG(INFO) << getSocketConnection()->getConnectionName() << "   HTTP/" << response->httpMajor << "." << response->httpMinor << " "
                  << response->statusCode << " " << response->reason;

        request->deliverResponse(request, response);

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") completed: " << requestLine;

        requestCompleted(response);
    }

    void SocketContext::deliverResponseParseError(int status, const std::string& reason) {
        const std::shared_ptr<MasterRequest> request = std::move(deliveredRequests.front());
        deliveredRequests.pop_front();

        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Response parse error: " << reason << " (" << status
                     << ") for request (" << request->count << "): " << requestLine
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

    void SocketContext::requestCompleted(const std::shared_ptr<Response>& response) {
        httpClose = response->connectionState == ConnectionState::Close ||
                    (response->connectionState == ConnectionState::Default &&
                     ((response->httpMajor == 0 && response->httpMinor == 0) || (response->httpMajor == 1 && response->httpMinor == 0)));

        if (httpClose) {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";

            shutdownWrite();
        } else {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";

            if (!pipelinedRequests && !pendingRequests.empty()) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        const std::shared_ptr<Request>& request = pendingRequests.front();

                        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Initiating request (" << request->count
                                   << "): " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "."
                                   << request->httpMinor;

                        initiateRequest();
                    }
                });
            }
        }
    }

    void SocketContext::setSseEventReceiver(const std::function<std::size_t()>& onServerSentEvent) {
        this->onServerSentEvent = onServerSentEvent;

        getSocketConnection()->setWriteTimeout(0);
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";

        onHttpConnected(masterRequest);
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        if (!httpClose && (!deliveredRequests.empty() || onServerSentEvent)) {
            if (!onServerSentEvent) {
                consumed = parser.parse();
            } else if (onServerSentEvent) {
                consumed = onServerSentEvent();
            }
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        while (!deliveredRequests.empty()) {
            const std::shared_ptr<Response> response(new Response());
            response->httpVersion = "HTTP/1.1";
            response->httpMajor = 1;
            response->httpMinor = 1;
            response->statusCode = "0";
            response->reason = "Connection loss";

            deliverResponse(response);
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
