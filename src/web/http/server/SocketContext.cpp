/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/http/server/SocketContext.h" // IWYU pragma: export

#include "core/EventReceiver.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/StatusCodes.h"
#include "web/http/server/Response.h"
#include "web/http/server/SemanticLog.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/SemanticLogger.h"
#include "utils/Timeval.h"

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
                  parsingRequestId = ++nextRequestId;
                  parsingRequest = true;
                  semantic::httpServerLog(*getSocketConnection()).debug() << "request started: id=" << parsingRequestId;
              },
              [this](web::http::server::Request&& request) {
                  frameworkLog().debug() << "HTTP: Request parse success: " << request.method << " " << request.url << " HTTP/"
                                         << request.httpMajor << "." << request.httpMinor;

                  pendingRequests.push_back({std::make_shared<Request>(std::move(request)), parsingRequestId, false});
                  parsingRequest = false;

                  if (pendingRequests.size() == 1) {
                      deliverRequest();
                  }
              },
              [this](int status, const std::string& reason) {
                  frameworkLog().error() << "HTTP: Request parse error: " << reason << " (" << status << ") ";
                  if (parsingRequest) {
                      semantic::httpServerLog(*getSocketConnection()).debug() << "request failed: id=" << parsingRequestId;
                  }
                  parsingRequest = false;
                  shutdownRead();

                  pendingRequests.push_back({std::make_shared<Request>(Request(status, reason)), parsingRequestId, true});

                  if (pendingRequests.size() == 1) {
                      deliverRequest();
                  }
              }) {
    }

    SocketContext* SocketContext::setOnConnected(std::function<void()> onConnectEventReceiver) {
        onConnectEventReceiverList.push_back(std::move(onConnectEventReceiver));

        return this;
    }

    SocketContext* SocketContext::setOnDisconnected(std::function<void()> onDisconnectEventReceiver) {
        onDisconnectEventReceiverList.push_back(std::move(onDisconnectEventReceiver));

        return this;
    }

    void SocketContext::deliverRequest() {
        if (!pendingRequests.empty()) {
            const std::shared_ptr<Request>& pendingRequest = pendingRequests.front().request;

            if (pendingRequest->status == 0) {
                frameworkLog().debug() << "HTTP: Request deliver: " << pendingRequest->method << " " << pendingRequest->url << " HTTP/"
                                       << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;

                masterResponse->init();
                masterResponse->requestMethod = pendingRequest->method;
                masterResponse->httpMajor = pendingRequest->httpMajor;
                masterResponse->httpMinor = pendingRequest->httpMinor;

                const std::string connection = pendingRequest->get("Connection");
                if (!connection.empty()) {
                    masterResponse->set("Connection", connection);
                }

                onRequestReady(pendingRequest, masterResponse);
            } else {
                masterResponse->init();
                masterResponse->httpMajor = 1;
                masterResponse->httpMinor = 1;

                masterResponse->set("Connection", "close");
                masterResponse->connectionState = ConnectionState::Close;

                masterResponse->status(pendingRequest->status).send(pendingRequest->reason);
            }
        } else {
            frameworkLog().trace() << "HTTP: No more pending request";
        }
    }

    void SocketContext::responseStarted(const Response& response) {
        if (!pendingRequests.empty()) {
            const std::shared_ptr<Request>& pendingRequest = pendingRequests.front().request;

            serverSentEvent = web::http::ciContains(pendingRequest->get("Accept"), "text/event-stream");

            if (serverSentEvent) {
                getSocketConnection()->setReadTimeout(0);
            }

            frameworkLog().debug() << "HTTP: Response start for request: " << pendingRequest->method << " " << pendingRequest->url
                                   << " HTTP/" << pendingRequest->httpMajor << "." << pendingRequest->httpMinor;
            frameworkLog().debug() << "  "
                                   << "HTTP/" + std::to_string(response.httpMajor)
                                                    .append(".")
                                                    .append(std::to_string(response.httpMinor))
                                                    .append(" ")
                                                    .append(std::to_string(response.statusCode))
                                                    .append(" ")
                                                    .append(StatusCode::reason(response.statusCode));
        }
    }

    void SocketContext::responseCompleted(const Response& response, bool success) {
        if (success) {
            requestCompleted(response);
        } else {
            frameworkLog().warn() << "HTTP: Response completed with error: " << response.statusCode << " "
                                  << StatusCode::reason(response.statusCode);

            if (!pendingRequests.empty() && !pendingRequests.front().terminal) {
                requestTerminal(pendingRequests.front(), "failed");
                pendingRequests.front().terminal = true;
            }

            close();
        }
    }

    void SocketContext::requestCompleted(const Response& response) {
        PendingRequest pendingRequest = std::move(pendingRequests.front());
        pendingRequests.pop_front();

        if (!pendingRequest.terminal) {
            requestTerminal(pendingRequest, "completed");
        }
        frameworkLog().debug() << "  "
                               << "HTTP/" + std::to_string(response.httpMajor)
                                                .append(".")
                                                .append(std::to_string(response.httpMinor))
                                                .append(" ")
                                                .append(std::to_string(response.statusCode))
                                                .append(" ")
                                                .append(StatusCode::reason(response.statusCode));

        httpClose = response.connectionState == ConnectionState::Close ||
                    (response.connectionState == ConnectionState::Default &&
                     ((response.httpMajor == 0 && response.httpMinor == 9) || (response.httpMajor == 1 && response.httpMinor == 0)));

        if (httpClose) {
            frameworkLog().debug() << "HTTP: Connection = Close";

            shutdownWrite();
        } else {
            frameworkLog().debug() << "HTTP: Connection = Keep-Alive";

            if (!pendingRequests.empty()) {
                core::EventReceiver::atNextTick([response = std::weak_ptr<Response>(masterResponse)]() {
                    if (!response.expired()) {
                        SocketContext* socketContext = response.lock()->getSocketContext();

                        if (socketContext != nullptr) {
                            socketContext->deliverRequest();
                        }
                    }
                });
            }
        }
    }

    void SocketContext::requestTerminal(const PendingRequest& request, const char* outcome) {
        semantic::httpServerLog(*getSocketConnection()).debug() << "request " << outcome << ": id=" << request.id;
    }

    void SocketContext::onConnected() {
        frameworkLog().debug() << "HTTP: context attached";

        for (const auto& onConnectEventReceiver : onConnectEventReceiverList) {
            onConnectEventReceiver();
        }
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = 0;

        if (!serverSentEvent && !httpClose) {
            consumed = parser.parse();
        }

        return consumed;
    }

    void SocketContext::onDisconnected() {
        masterResponse->disconnect();

        if (parsingRequest) {
            semantic::httpServerLog(*getSocketConnection()).debug() << "request aborted: id=" << parsingRequestId;
            parsingRequest = false;
        }

        for (PendingRequest& request : pendingRequests) {
            if (!request.terminal) {
                requestTerminal(request, "aborted");
                request.terminal = true;
            }
        }

        frameworkLog().debug() << (getDetachReason() == DetachReason::ContextSwitch ? "HTTP: context detached for context switch"
                                                                                     : "HTTP: context detached for connection close");

        for (const auto& onDisconnectEventReceiver : onDisconnectEventReceiverList) {
            onDisconnectEventReceiver();
        }
    }

    bool SocketContext::onSignal([[maybe_unused]] int signum) {
        return true;
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        // Do nothing in case of an write error
    }

} // namespace web::http::server
