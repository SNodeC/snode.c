/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/EventReceiver.h"
#include "web/http/http_utils.h"
#include "web/http/server/SocketContext.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename Request, typename Response>
    SocketContext<Request, Response>::SocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        const std::function<void(std::shared_ptr<Request>&, std::shared_ptr<Response>&)>& onRequestReady)
        : Super(socketConnection)
        , onRequestReady(onRequestReady)
        , response(std::make_shared<Response>(this))
        , parser(
              this,
              [this](web::http::server::Request& request) -> void {
                  for (const auto& [field, value] : request.headers) {
                      if (field == "connection" && httputils::ci_contains(value, "close")) {
                          request.connectionState = ConnectionState::Close;
                      } else if (field == "connection" && httputils::ci_contains(value, "keep-alive")) {
                          request.connectionState = ConnectionState::Keep;
                      }
                  }

                  requests.emplace_back(std::make_shared<Request>(std::move(request)));

                  requestParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  requestError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceivedFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestParsed() {
        if (request == nullptr && !requests.empty()) {
            request = requests.front();
            requests.pop_front();

            bool close = (request->httpMajor == 0 && request->httpMinor == 9) ||
                         (request->httpMajor == 1 && request->httpMinor == 0 && request->connectionState != ConnectionState::Keep) ||
                         (request->httpMajor == 1 && request->httpMinor == 1 && request->connectionState == ConnectionState::Close);
            if (close) {
                response->set("Connection", "close");
            } else {
                response->set("Connection", "keep-alive");
            }

            onRequestReady(request, response);
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestError(int status, const std::string& reason) {
        response->status(status).send(reason);

        shutdownWrite(true);
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestCompleted() {
        // if 0.9 => terminate
        // if 1.0 && (request != Keep || contentLength = -1) => terminate
        // if 1.1 && (request == Close || contentLength = -1) => terminate
        // if (request == Close) => terminate

        if (request != nullptr) {
            bool close = (request->httpMajor == 0 && request->httpMinor == 9) ||
                         (request->httpMajor == 1 && request->httpMinor == 0 && request->connectionState != ConnectionState::Keep) ||
                         (request->httpMajor == 1 && request->httpMinor == 1 && request->connectionState == ConnectionState::Close) ||
                         response->connectionState == ConnectionState::Close;
            if (close) {
                shutdownWrite();
            } else if (!requests.empty()) {
                core::EventReceiver::atNextTick([this]() -> void {
                    requestParsed();
                });
            }

            response->reset();
            request = nullptr;
        } else {
            shutdownWrite();
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        if (response != nullptr) {
            response->stopResponse();
        }

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onDisconnected";
    }

    template <typename Request, typename Response>
    bool SocketContext<Request, Response>::onSignal(int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onSignal " << signum;

        return true;
    }

} // namespace web::http::server
