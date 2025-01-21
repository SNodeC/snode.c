/*
 * SNode.C - a slim toolkit for network communication
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
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestBegin,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd,
                                 bool pipelinedRequests)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , onRequestEnd(onRequestEnd)
        , pipelinedRequests(pipelinedRequests)
        , masterRequest(std::make_shared<Request>(this, getSocketConnection()->getConfiguredServer()))
        , parser(
              this,
              [this]() {
                  responseStarted();
              },
              [this](web::http::client::Response&& response) {
                  deliverResponse(std::move(response));
              },
              [this](int status, const std::string& reason) {
                  deliverResponseParseError(status, reason);
              }) {
        masterRequest->setMasterRequest(masterRequest);
    }

    void SocketContext::requestPrepared(Request&& request) {
        const std::string requestLine = std::string(request.method)
                                            .append(" ")
                                            .append(request.url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request.httpMajor))
                                            .append(".")
                                            .append(std::to_string(request.httpMinor));

        if ((flags == Flags::NONE || (flags & Flags::HTTP11) == Flags::HTTP11 || (flags & Flags::KEEPALIVE) == Flags::KEEPALIVE) &&
            (flags & Flags::CLOSE) != Flags::CLOSE) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request accepted: " << requestLine;

            flags = (flags & ~Flags::HTTP11) | ((request.httpMajor == 1 && request.httpMinor == 1) ? Flags::HTTP11 : Flags::NONE);
            flags = (flags & ~Flags::HTTP10) | ((request.httpMajor == 1 && request.httpMinor == 0) ? Flags::HTTP10 : Flags::NONE);
            flags = (flags & ~Flags::KEEPALIVE) |
                    (web::http::ciContains(request.header("Connection"), "keep-alive") ? Flags::KEEPALIVE : Flags::NONE);
            flags = (flags & ~Flags::CLOSE) | (web::http::ciContains(request.header("Connection"), "close") ? Flags::CLOSE : Flags::NONE);

            if (!currentRequest) {
                initiateRequest(request);
            } else {
                LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request queued: " << requestLine;
                pendingRequests.emplace_back(std::move(request));
            }
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request rejected: " << requestLine;
        }
    }

    void SocketContext::initiateRequest(Request& request) {
        currentRequest = std::make_shared<Request>(std::move(request));

        const std::string requestLine = std::string(currentRequest->method)
                                            .append(" ")
                                            .append(currentRequest->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(currentRequest->httpMajor))
                                            .append(".")
                                            .append(std::to_string(currentRequest->httpMinor));

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request initiating: " << requestLine;

        if (currentRequest->initiate()) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request initiated: " << requestLine;
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request initiating failed: " << requestLine;

            core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                if (!masterRequest.expired()) {
                    if (!pendingRequests.empty()) {
                        initiateRequest(pendingRequests.front());
                        pendingRequests.pop_front();
                    } else {
                        currentRequest = nullptr;
                    }
                }
            });
        }
    }

    void SocketContext::requestDelivered(Request&& request, bool success) {
        const std::string requestLine = std::string(request.method)
                                            .append(" ")
                                            .append(request.url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request.httpMajor))
                                            .append(".")
                                            .append(std::to_string(request.httpMinor));

        if (success) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request delivered: " << requestLine;

            deliveredRequests.emplace_back(std::move(request));

            if (pipelinedRequests) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        if (!pendingRequests.empty()) {
                            initiateRequest(pendingRequests.front());
                            pendingRequests.pop_front();
                        } else {
                            currentRequest = nullptr;
                        }
                    }
                });
            }
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request delivering failed: " << requestLine;

            shutdownWrite();
        }
    }

    void SocketContext::responseStarted() {
        if (!deliveredRequests.empty()) {
            const std::string requestLine = std::string(deliveredRequests.front().method)
                                                .append(" ")
                                                .append(deliveredRequests.front().url)
                                                .append(" HTTP/")
                                                .append(std::to_string(deliveredRequests.front().httpMajor))
                                                .append(".")
                                                .append(std::to_string(deliveredRequests.front().httpMinor));

            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started: " << requestLine;
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response without delivered request";
            shutdownWrite(true);
        }
    }

    void SocketContext::deliverResponse(Response&& response) {
        const std::shared_ptr<Request> request = std::make_shared<Request>(std::move(deliveredRequests.front()));
        deliveredRequests.pop_front();

        const std::string requestLine = std::string(request->method)
                                            .append(" ")
                                            .append(request->url)
                                            .append(" HTTP/")
                                            .append(std::to_string(request->httpMajor))
                                            .append(".")
                                            .append(std::to_string(request->httpMinor));

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response parsed: " << requestLine;

        const bool httpClose =
            response.connectionState == ConnectionState::Close ||
            (response.connectionState == ConnectionState::Default &&
             ((response.httpMajor == 0 && response.httpMinor == 9) || (response.httpMajor == 1 && response.httpMinor == 0)));

        request->deliverResponse(request, std::make_shared<Response>(std::move(response)));

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response delivered: " << requestLine;

        responseDelivered(httpClose);
    }

    void SocketContext::deliverResponseParseError(int status, const std::string& reason) {
        const std::shared_ptr<Request> request = std::make_shared<Request>(std::move(deliveredRequests.front()));
        deliveredRequests.pop_front();

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response parse error: " << reason << " (" << status << ") "
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
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Close";
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Keep-Alive";

            if (!pipelinedRequests) {
                core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() {
                    if (!masterRequest.expired()) {
                        if (!pendingRequests.empty()) {
                            initiateRequest(pendingRequests.front());
                            pendingRequests.pop_front();
                        } else {
                            currentRequest = nullptr;
                        }
                    }
                });
            }
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: connected";

        onRequestBegin(masterRequest);
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
            const Request& request = deliveredRequests.front();
            if (request.httpMajor == 1 && request.httpMinor == 0) {
                deliverResponse(parser.getResponse());
            }
        }

        onRequestEnd(masterRequest);

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: disconnected";
    }

    bool SocketContext::onSignal([[maybe_unused]] int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: received signal " << signum;

        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::client
