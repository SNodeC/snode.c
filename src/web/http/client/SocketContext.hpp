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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename Request, typename Response>
    SocketContext<Request, Response>::SocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        const std::function<void(std::shared_ptr<Request>&)>& onRequestBegin,
        const std::function<void(std::shared_ptr<Request>&, std::shared_ptr<Response>&)>& onResponseReady,
        const std::function<void(int, const std::string&)>& onResponseError)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , onResponseReady(onResponseReady)
        , onResponseError(onResponseError)
        , request(std::make_shared<Request>(this))
        , parser(
              this,
              [onResponseReady, this](web::http::client::Response& response) -> void {
                  this->response = std::make_shared<Response>(std::move(response));

                  responseParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  responseError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted(bool success) {
        if (success) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent successful";
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent failed";

            shutdownWrite();
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::responseParsed() {
        onResponseReady(request, response);

        requestCompleted();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::responseError(int status, const std::string& reason) {
        onResponseError(status, reason);

        shutdownWrite(true);
    }

    template <typename RequestT, typename ResponseT>
    void SocketContext<RequestT, ResponseT>::requestCompleted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request completed successful";

        bool close = response->connectionState == ConnectionState::Close ||
                     (response->connectionState == ConnectionState::Default &&
                      ((request->httpMajor == 0 && request->httpMinor == 9) || (request->httpMajor == 1 && request->httpMinor == 0)));

        if (close) {
            shutdownWrite();
        } else {
            // TODO: Queue requests
            // TODO: Process next queued request
        }

        response = nullptr;
        request->reInit();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";

        request->host(getSocketConnection()->getConfiguredServer());

        onRequestBegin(request);
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceivedFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        if (request != nullptr) {
            request->stopResponse();
        }

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onDisconnected";
    }

    template <typename Request, typename Response>
    bool SocketContext<Request, Response>::onSignal([[maybe_unused]] int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onSignal  " << signum;

        return true;
    }

} // namespace web::http::client
