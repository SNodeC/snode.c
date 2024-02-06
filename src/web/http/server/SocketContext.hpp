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

#include "web/http/ConnectionState.h"
#include "web/http/http_utils.h"
#include "web/http/server/SocketContext.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename Request, typename Response>
    SocketContext<Request, Response>::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                                    const std::function<void(Request&, Response&)>& onRequestReady)
        : Super(socketConnection)
        , onRequestReady(onRequestReady)
        , parser(
              this,
              [this]() -> void {
                  requestContexts.emplace_back(new RequestContext(this));
              },
              [&requestContexts = this->requestContexts](const std::string& method,
                                                         const std::string& url,
                                                         const std::string& httpVersion,
                                                         int httpMajor,
                                                         int httpMinor,
                                                         std::map<std::string, std::string>& queries) -> void {
                  Request& request = requestContexts.back()->request;

                  request.method = method;
                  request.url = url;
                  request.queries = std::move(queries);
                  request.httpVersion = httpVersion;
                  request.httpMajor = httpMajor;
                  request.httpMinor = httpMinor;
              },
              [&requestContexts = this->requestContexts](std::map<std::string, std::string>& header,
                                                         std::map<std::string, std::string>& cookies) -> void {
                  Request& request = requestContexts.back()->request;

                  request.headers = std::move(header);

                  for (auto& [field, value] : request.headers) {
                      if (field == "connection" && httputils::ci_contains(value, "close")) {
                          request.connectionState = ConnectionState::Close;
                      } else if (field == "connection" && httputils::ci_contains(value, "keep-alive")) {
                          request.connectionState = ConnectionState::Keep;
                      }
                  }

                  request.cookies = std::move(cookies);
              },
              [&requestContexts = this->requestContexts](std::vector<uint8_t>& content) -> void {
                  Request& request = requestContexts.back()->request;

                  request.body = std::move(content);
              },
              [this]() -> void {
                  RequestContext* requestContext = requestContexts.back();

                  requestContext->ready = true;

                  requestParsed();
              },
              [this](int status, std::string&& reason) -> void {
                  RequestContext* requestContext = requestContexts.back();

                  requestContext->status = status;
                  requestContext->reason = std::move(reason);
                  requestContext->ready = true;

                  requestParsed();
              }) {
    }

    template <typename Request, typename Response>
    SocketContext<Request, Response>::~SocketContext() {
        for (RequestContext* requestContext : requestContexts) {
            delete requestContext;
        }
        if (currentRequestContext != nullptr) {
            currentRequestContext->stop();
        }
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceivedFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestParsed() {
        if (!currentRequestContext) {
            currentRequestContext = requestContexts.front();

            if (currentRequestContext != nullptr && currentRequestContext->ready) {
                requestContexts.pop_front();

                Response& response = currentRequestContext->response;

                if (currentRequestContext->status == 0) {
                    Request& request = currentRequestContext->request;

                    bool close = (request.httpMajor == 0 && request.httpMinor == 9) ||
                                 (request.httpMajor == 1 && request.httpMinor == 0 && request.connectionState != ConnectionState::Keep) ||
                                 (request.httpMajor == 1 && request.httpMinor == 1 && request.connectionState == ConnectionState::Close);

                    if (close) {
                        response.set("Connection", "close");
                    } else {
                        response.set("Connection", "keep-alive");
                    }

                    onRequestReady(request, response);
                } else {
                    int status = currentRequestContext->status;
                    std::string& reason = currentRequestContext->reason;

                    response.status(status).send(reason);
                    delete currentRequestContext;
                    currentRequestContext = nullptr;

                    shutdownWrite(true);
                }
            }
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted() {
        // if 0.9 => terminate
        // if 1.0 && (request != Keep || contentLength = -1) => terminate
        // if 1.1 && (request == Close || contentLength = -1) => terminate
        // if (request == Close) => terminate

        if (currentRequestContext != nullptr) {
            const Response& response = currentRequestContext->response;
            const Request& request = currentRequestContext->request;

            currentRequestContext = nullptr;

            bool close = (request.httpMajor == 0 && request.httpMinor == 9) ||
                         (request.httpMajor == 1 && request.httpMinor == 0 && request.connectionState != ConnectionState::Keep) ||
                         (request.httpMajor == 1 && request.httpMinor == 1 && request.connectionState == ConnectionState::Close) ||
                         response.connectionState == ConnectionState::Close;

            if (close) {
                shutdownWrite();
            } else if (!requestContexts.empty() && requestContexts.front()->ready) {
                requestParsed();
            }
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << ": HTTP onConnected";
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        LOG(INFO) << getSocketConnection()->getInstanceName() << ": HTTP onDisconnected";
    }

    template <typename Request, typename Response>
    bool SocketContext<Request, Response>::onSignal(int signum) {
        LOG(INFO) << getSocketConnection()->getInstanceName() << ": HTTP onSignal: " << signum;

        return true;
    }

} // namespace web::http::server
