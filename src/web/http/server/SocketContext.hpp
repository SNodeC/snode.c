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
                  std::string connection = request.get("Connection");
                  if (!connection.empty()) {
                      response->set("Connection", connection);
                  }

                  requests.emplace_back(std::make_shared<Request>(std::move(request)));

                  requestParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  requestError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestParsed() {
        if (request == nullptr && !requests.empty()) { // cppcheck-suppress accessMoved
            request = requests.front();
            requests.pop_front();

            onRequestReady(request, response);
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestError(int status, const std::string& reason) {
        response->status(status).send(reason);

        shutdownWrite(true);
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted(bool success) {
        if (success) {
            requestCompleted();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request: wrong content length";

            shutdownWrite();
        }
    }

    template <typename RequestT, typename ResponseT>
    void SocketContext<RequestT, ResponseT>::requestCompleted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request completed successful";

        bool close = response->connectionState == ConnectionState::Close ||
                     (response->connectionState == ConnectionState::Default &&
                      ((request->httpMajor == 0 && request->httpMinor == 9) || (request->httpMajor == 1 && request->httpMinor == 0)));

        if (close) {
            shutdownWrite();
        } else if (!requests.empty()) {
            core::EventReceiver::atNextTick([this]() -> void {
                requestParsed();
            });
        }

        request = nullptr;
        response->reInit();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceivedFromPeer() {
        return parser.parse();
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
