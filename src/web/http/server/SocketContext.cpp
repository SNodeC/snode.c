/*
 * snode.c - a slim toolkit for network communication
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

#include "web/http/server/SocketContext.h" // IWYU pragma: export

#include "core/EventReceiver.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/Response.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContext::SocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onRequestReady)
        : Super(socketConnection)
        , onRequestReady(onRequestReady)
        , response(std::make_shared<Response>(this))
        , parser(
              this,
              [this]() -> void {
                  requestStarted();
              },
              [this](web::http::server::Request&& request) -> void {
                  requestParsed(std::move(request));
              },
              [this](int status, const std::string& reason) -> void {
                  requestParseError(status, reason);
              }) {
    }

    void SocketContext::requestStarted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request started";
    }

    void SocketContext::requestParsed(web::http::server::Request&& request) {
        if (requests.empty()) {
            deliverRequest(std::move(request));
        } else {
            requests.emplace_back(std::move(request));
        }
    }

    void SocketContext::requestParseError(int status, const std::string& reason) {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request parse error: " << status << " : " << reason;

        response->status(status).send(reason);

        shutdownWrite(true);
    }

    void SocketContext::deliverRequest(web::http::server::Request&& request) {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request parsed";

        const std::string connection = request.get("Connection");
        if (!connection.empty()) {
            response->set("Connection", connection);
        }
        response->httpMajor = request.httpMajor;
        response->httpMinor = request.httpMinor;

        onRequestReady(std::make_shared<Request>(std::move(request)), response);
    }

    void SocketContext::responseStarted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started";
    }

    void SocketContext::responseCompleted(bool success) {
        if (success) {
            httpClose = response->connectionState == ConnectionState::Close ||
                        (response->connectionState == ConnectionState::Default && ((response->httpMajor == 0 && response->httpMinor == 9) ||
                                                                                   (response->httpMajor == 1 && response->httpMinor == 0)));

            requestCompleted();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response wrong content length";

            shutdownWrite();
        }

        response->init();
    }

    void SocketContext::requestCompleted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request completed";

        if (httpClose) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Close";

            shutdownWrite();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Keep-Alive";

            if (!requests.empty()) {
                core::EventReceiver::atNextTick([this]() -> void {
                    deliverRequest(std::move(requests.front()));
                    requests.pop_front();
                });
            }
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";

        response->init();
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        VLOG(0) << "+++++++++++++++++++++++++++++";

        if (!httpClose) {
            consumed = parser.parse();
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        if (response != nullptr) {
            response->stopResponse();
        }

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onDisconnected";
    }

    bool SocketContext::onSignal(int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onSignal " << signum;

        return true;
    }

} // namespace web::http::server
