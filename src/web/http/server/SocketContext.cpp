/*
 * snode.c - a slim toolkit for network communication
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

#include "web/http/server/SocketContext.h" // IWYU pragma: export

#include "core/EventReceiver.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/Response.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContext::SocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onRequestReady)
        : Super(socketConnection)
        , onRequestReady(onRequestReady)
        , masterResponse(std::make_shared<Response>(this))
        , parser(
              this,
              [this]() -> void {
                  requestStarted();
              },
              [this](web::http::server::Request&& request) -> void {
                  if (!currentRequest) {
                      deliverRequest(request);
                  } else {
                      pendingRequests.emplace_back(std::move(request));
                  }
              },
              [this](int status, const std::string& reason) -> void {
                  LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request parse error: " << reason << " (" << status
                             << ") ";

                  masterResponse->status(status).send(reason);

                  shutdownWrite(true);
              }) {
    }

    void SocketContext::requestStarted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request started";
    }

    void SocketContext::deliverRequest(web::http::server::Request& request) {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request parsed " << request.method << " " << request.url;

        currentRequest = std::make_shared<Request>(std::move(request));

        masterResponse->init();
        masterResponse->httpMajor = currentRequest->httpMajor;
        masterResponse->httpMinor = currentRequest->httpMinor;
        const std::string connection = currentRequest->get("Connection");
        if (!connection.empty()) {
            masterResponse->set("Connection", connection);
        }

        onRequestReady(currentRequest, masterResponse);
    }

    void SocketContext::responseStarted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started";
    }

    void SocketContext::responseCompleted(bool success) {
        if (success) {
            httpClose = masterResponse->connectionState == ConnectionState::Close ||
                        (masterResponse->connectionState == ConnectionState::Default &&
                         ((masterResponse->httpMajor == 0 && masterResponse->httpMinor == 9) ||
                          (masterResponse->httpMajor == 1 && masterResponse->httpMinor == 0)));

            requestCompleted();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response wrong content length";

            shutdownWrite(true);
        }
    }

    void SocketContext::requestCompleted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request completed";

        if (httpClose) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Close";

            shutdownWrite();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Keep-Alive";

            core::EventReceiver::atNextTick([this, response = static_cast<std::weak_ptr<Response>>(this->masterResponse)]() -> void {
                if (!response.expired()) {
                    if (!pendingRequests.empty()) {
                        deliverRequest(pendingRequests.front());
                        pendingRequests.pop_front();
                    } else {
                        currentRequest = nullptr;
                    }
                }
            });
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: Connected";

        masterResponse->init();
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        if (!httpClose) {
            consumed = parser.parse();
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        if (masterResponse != nullptr) {
            masterResponse->stopResponse();
        }

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: Disconnected";
    }

    bool SocketContext::onSignal(int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: Received signal " << signum;

        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::server
