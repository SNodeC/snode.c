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

#include "core/EventReceiver.h"
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
        const std::function<void(int, const std::string&)>& onResponseParseError,
        const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , onResponseReady(onResponseReady)
        , onResponseParseError(onResponseParseError)
        , onRequestEnd(onRequestEnd)
        , masterRequest(std::make_shared<Request>(this))
        , parser(
              this,
              [onResponseReady, this](web::http::client::Response& response) -> void {
                  responses.emplace_back(std::make_shared<Response>(std::move(response)));

                  responseParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  responseError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerStarted() {
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted(bool success) {
        if (success) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent successful";

            requests.emplace_back(std::make_shared<Request>(std::move(*masterRequest)));
            masterRequest->reInit();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent failed";

            shutdownWrite();
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::responseParsed() {
        if (currentResponse == nullptr && !responses.empty()) {
            currentRequest = requests.front();
            requests.pop_front();
            currentResponse = responses.front();
            responses.pop_front();

            onResponseReady(currentRequest, currentResponse);

            requestCompleted();
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::responseError(int status, const std::string& reason) {
        onResponseParseError(status, reason);

        shutdownWrite(true);
    }

    template <typename RequestT, typename ResponseT>
    void SocketContext<RequestT, ResponseT>::requestCompleted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request completed successful";

        bool close = currentResponse->connectionState == ConnectionState::Close ||
                     (currentResponse->connectionState == ConnectionState::Default &&
                      ((currentRequest->httpMajor == 0 && currentRequest->httpMinor == 9) ||
                       (currentRequest->httpMajor == 1 && currentRequest->httpMinor == 0)));

        if (close) {
            shutdownWrite();
        } else if (!responses.empty()) {
            core::EventReceiver::atNextTick([this]() -> void {
                responseParsed();
            });
        }

        currentRequest = nullptr;
        currentResponse = nullptr;
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";

        masterRequest->host(getSocketConnection()->getConfiguredServer());

        onRequestBegin(masterRequest);
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceivedFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        if (currentRequest != nullptr) {
            currentRequest->stopResponse();
        }

        onRequestEnd(masterRequest);

        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onDisconnected";
    }

    template <typename Request, typename Response>
    bool SocketContext<Request, Response>::onSignal([[maybe_unused]] int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onSignal  " << signum;

        return true;
    }

} // namespace web::http::client
