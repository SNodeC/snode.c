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
              [this]() {
                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Request started";
              },
              [this](web::http::server::Request&& request) {
                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Request parsed: " << request.method << " "
                            << request.url << " HTTP/" << request.httpMajor << "." << request.httpMinor;

                  pendingRequests.emplace_back(std::make_shared<Request>(std::move(request)));

                  if (pendingRequests.size() == 1) {
                      deliverRequest();
                  }
              },
              [this](int status, const std::string& reason) {
                  LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP Request parse error: " << reason << " (" << status
                             << ") ";

                  shutdownWrite(true);
              }) {
    }

    void SocketContext::deliverRequest() {
        if (!pendingRequests.empty()) {
            const std::shared_ptr<Request>& pendingRequest = pendingRequests.front();

            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Request ready: " << pendingRequest->method << " "
                      << pendingRequest->url << " HTTP/" << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;

            masterResponse->init();
            masterResponse->httpMajor = pendingRequest->httpMajor;
            masterResponse->httpMinor = pendingRequest->httpMinor;

            const std::string connection = pendingRequest->get("Connection");
            if (!connection.empty()) {
                masterResponse->set("Connection", connection);
            }

            onRequestReady(pendingRequest, masterResponse);
        } else {
            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Request: No more pending";
        }
    }

    void SocketContext::responseStarted() {
        const std::shared_ptr<Request>& pendingRequest = pendingRequests.front();

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Response started: " << pendingRequest->method << " "
                  << pendingRequest->url << " HTTP/" << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;
    }

    void SocketContext::responseCompleted(bool success) {
        if (success) {
            const std::shared_ptr<Request>& pendingRequest = pendingRequests.front();

            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Response completed: " << pendingRequest->method << " "
                      << pendingRequest->url << " HTTP/" << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;

            httpClose = masterResponse->connectionState == ConnectionState::Close ||
                        (masterResponse->connectionState == ConnectionState::Default &&
                         ((masterResponse->httpMajor == 0 && masterResponse->httpMinor == 9) ||
                          (masterResponse->httpMajor == 1 && masterResponse->httpMinor == 0)));

            requestCompleted();
        } else {
            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP Response wrong content length";

            shutdownWrite(true);
        }
    }

    void SocketContext::requestCompleted() {
        const std::shared_ptr<Request>& pendingRequest = pendingRequests.front();

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP Request completed: " << pendingRequest->method << " "
                  << pendingRequest->url << " HTTP/" << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;

        if (httpClose) {
            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";

            shutdownWrite();
        } else {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";

            core::EventReceiver::atNextTick([this, response = static_cast<std::weak_ptr<Response>>(this->masterResponse)]() {
                if (!response.expired()) {
                    pendingRequests.pop_front();
                    deliverRequest();
                }
            });
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";

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

        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Disconnected";
    }

    bool SocketContext::onSignal(int signum) {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received signal " << signum;

        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::server
