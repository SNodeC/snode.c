/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

namespace web::http::client {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestBegin,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , onRequestEnd(onRequestEnd)
        , masterRequest(std::make_shared<Request>(this, getSocketConnection()->getConfiguredServer()))
        , parser(
              this,
              [this]() -> void {
                  responseStarted();
              },
              [this](web::http::client::Response&& response) -> void {
                  deliverResponse(std::move(response));
              },
              [this](int status, const std::string& reason) -> void {
                  deliverResponseParseError(status, reason);
              }) {
        masterRequest->setMasterRequest(masterRequest);
    }

    void SocketContext::requestPrepared(Request& request) {
        if ((flags == Flags::NONE || (flags & Flags::HTTP11) == Flags::HTTP11 || (flags & Flags::KEEPALIVE) == Flags::KEEPALIVE) &&
            (flags & Flags::CLOSE) != Flags::CLOSE) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request accepted: " << request.method << " " << request.url
                       << " HTTP/" << request.httpMajor << "." << request.httpMinor;

            flags = (flags & ~Flags::HTTP11) | ((request.httpMajor == 1 && request.httpMinor == 1) ? Flags::HTTP11 : Flags::NONE);
            flags = (flags & ~Flags::HTTP10) | ((request.httpMajor == 1 && request.httpMinor == 0) ? Flags::HTTP10 : Flags::NONE);
            flags = (flags & ~Flags::KEEPALIVE) |
                    (web::http::ciContains(request.header("Connection"), "keep-alive") ? Flags::KEEPALIVE : Flags::NONE);
            flags = (flags & ~Flags::CLOSE) | (web::http::ciContains(request.header("Connection"), "close") ? Flags::CLOSE : Flags::NONE);

            if (!currentRequest) {
                deliverRequest(std::move(request));
            } else {
                pendingRequests.emplace_back(std::move(request));
            }
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request rejected: " << request.method << " " << request.url
                       << " HTTP/" << request.httpMajor << "." << request.httpMinor;
            std::make_shared<Request>(std::move(request));
        }
    }

    void SocketContext::deliverRequest(Request&& request) {
        currentRequest = std::make_shared<Request>(std::move(request));

        const std::string requestLine = currentRequest->method + " " + currentRequest->url + " HTTP/" +
                                        std::to_string(currentRequest->httpMajor) + "." + std::to_string(currentRequest->httpMinor);

        if (currentRequest->execute()) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request executed: " << requestLine;
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request execute failed: " << requestLine;

            core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() -> void {
                if (!masterRequest.expired()) {
                    if (!pendingRequests.empty()) {
                        deliverRequest(std::move(pendingRequests.front()));
                        pendingRequests.pop_front();
                    } else {
                        currentRequest = nullptr;
                    }
                }
            });
        }
    }

    void SocketContext::requestDelivered(Request&& request, bool success) {
        deliveredRequests.emplace_back(std::move(request));

        if (success) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request delivered: " << deliveredRequests.front().method
                       << " " << deliveredRequests.front().url << " HTTP/" << deliveredRequests.front().httpMajor << "."
                       << deliveredRequests.front().httpMinor;

            core::EventReceiver::atNextTick([this, masterRequest = static_cast<std::weak_ptr<Request>>(masterRequest)]() -> void {
                if (!masterRequest.expired()) {
                    if (!pendingRequests.empty()) {
                        deliverRequest(std::move(pendingRequests.front()));
                        pendingRequests.pop_front();
                    } else {
                        currentRequest = nullptr;
                    }
                }
            });
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent failed";

            shutdownWrite();
        }
    }

    void SocketContext::responseStarted() {
        if (!deliveredRequests.empty()) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started: "
                       << std::string(deliveredRequests.front().method)
                              .append(" ")
                              .append(deliveredRequests.front().url)
                              .append(" HTTP/")
                              .append(std::to_string(deliveredRequests.front().httpMajor))
                              .append(".")
                              .append(std::to_string(deliveredRequests.front().httpMinor));
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started without delivered request";
            shutdownWrite(true);
        }
    }

    void SocketContext::deliverResponse(Response&& response) {
        VLOG(0) << "FlyingSize: size = " << deliveredRequests.size();
        const std::shared_ptr<Request> request = std::make_shared<Request>(std::move(deliveredRequests.front()));
        deliveredRequests.pop_front();

        const std::string requestMethod = std::string(request->method)
                                              .append(" ")
                                              .append(request->url)
                                              .append(" HTTP/")
                                              .append(std::to_string(request->httpMajor))
                                              .append(".")
                                              .append(std::to_string(request->httpMinor));

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response parsed: " << requestMethod;

        const bool httpClose =
            response.connectionState == ConnectionState::Close ||
            (response.connectionState == ConnectionState::Default &&
             ((response.httpMajor == 0 && response.httpMinor == 9) || (response.httpMajor == 1 && response.httpMinor == 0)));

        request->deliverResponse(request, std::make_shared<Response>(std::move(response)));

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response delivered: " << requestMethod;

        // Starting from here the currentRequest may be invalid, due to reuse in user code during deliverResponse(...). This is
        // completely fine as the currentRequest is no more used anyway. An optional solution would be to pass the currentRequest as
        // std::shared_ptr<const Request> to not allow modifications in user code. In user code the masterRequest has to be used instead
        // in that case. Current decision is to allow reuse of the currentRequest in user code - why not?

        requestCompleted(httpClose);
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

        VLOG(0) << "##################### 4";
        shutdownWrite(true);
    }

    void SocketContext::requestCompleted(bool httpClose) {
        if (httpClose) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Close";

            VLOG(0) << "##################### 5";
            shutdownWrite();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Keep-Alive";
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: connected";

        masterRequest->init(getSocketConnection()->getConfiguredServer());

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
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: received signal  " << signum;

        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::client
