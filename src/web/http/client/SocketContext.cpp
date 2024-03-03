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
#include "web/http/client/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestBegin,
                                 const std::function<void(int, const std::string&)>& onResponseParseError,
                                 const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , onResponseParseError(onResponseParseError)
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
                  responseError(status, reason);
              }) {
        masterRequest->setMasterRequest(masterRequest);
    }

    void SocketContext::dispatchNextRequest() {
        if (!preparedRequests.empty()) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request dispatch next";

            Request* request = preparedRequests.front().get();
            if (request->execute()) {
                LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request start success: " << request->method << " "
                           << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;
            } else {
                LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request start failed: " << request->method << " "
                           << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;

                preparedRequests.pop_front();

                if (!preparedRequests.empty()) {
                    core::EventReceiver::atNextTick([this]() -> void {
                        dispatchNextRequest();
                    });
                }
            }
        }
    }

    void SocketContext::requestPrepared(Request& request) {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request prepared";

        preparedRequests.emplace_back(std::make_shared<Request>(std::move(request)));

        if (preparedRequests.size() == 1) {
            dispatchNextRequest();
        }
    }

    void SocketContext::requestSent(bool success) {
        sentRequests.emplace_back(preparedRequests.front());
        preparedRequests.pop_front();

        if (success) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent successful";

            if (!preparedRequests.empty()) {
                core::EventReceiver::atNextTick([this]() -> void {
                    dispatchNextRequest();
                });
            }
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Request sent failed";

            shutdownWrite();
        }
    }

    void SocketContext::responseStarted() {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response started: " << sentRequests.size();

        if (sentRequests.empty()) {
            shutdownWrite(true);
        }
    }

    void SocketContext::deliverResponse(Response&& response) {
        currentRequest = std::move(sentRequests.front());
        sentRequests.pop_front();

        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response parsed - deliver response: " << currentRequest->method
                   << " " << currentRequest->url << " HTTP/" << currentRequest->httpMajor << "." << currentRequest->httpMinor;

        httpClose = response.connectionState == ConnectionState::Close ||
                    (response.connectionState == ConnectionState::Default &&
                     ((response.httpMajor == 0 && response.httpMinor == 9) || (response.httpMajor == 1 && response.httpMinor == 0)));

        currentRequest->deliverResponse(currentRequest, std::make_shared<Response>(std::move(response)));

        // Starting from here the currentRequest may be invalid, due to reuse in user code during deliverResponse(...). This is completely
        // fine as the currentRequest is no more used anyway. An optional solution would be to pass the currentRequest as
        // std::shared_ptr<const Request> to not allow modifications in user code. In user code the masterRequest has to be used instead in
        // that case.
        // Current decision is to allow reuse of the currentRequest in user code - why not?

        requestCompleted();
    }

    void SocketContext::responseError(int status, const std::string& reason) {
        LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Response parse error: " << status << " : " << reason;

        currentRequest->deliverResponseParseError(currentRequest, reason);

        shutdownWrite(true);
    }

    void SocketContext::requestCompleted() {
        if (httpClose) {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Close";

            shutdownWrite();
        } else {
            LOG(TRACE) << getSocketConnection()->getInstanceName() << " HTTP: Connection = Keep-Alive";
        }
    }

    void SocketContext::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onConnected";

        masterRequest->init(getSocketConnection()->getConfiguredServer());

        onRequestBegin(masterRequest);
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        if (!sentRequests.empty()) {
            consumed = parser.parse();
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        if (!sentRequests.empty()) {
            currentRequest = std::move(sentRequests.front());
            sentRequests.pop_front();

            if (currentRequest->httpMajor == 1 && currentRequest->httpMinor == 0) {
                deliverResponse(parser.getResponse());
            }
        }

        onRequestEnd(masterRequest);
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onDisconnected";
    }

    bool SocketContext::onSignal([[maybe_unused]] int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << " HTTP: onSignal  " << signum;

        return true;
    }

} // namespace web::http::client
