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

#include "log/Logger.h"
#include "web/http/ConnectionState.h"
#include "web/http/http_utils.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename Request, typename Response>
    SocketContext<Request, Response>::SocketContext(core::socket::SocketConnection* socketConnection,
                                                    const std::function<void(Request&, Response&)>& onRequestReady)
        : Super(socketConnection)
        , onRequestReady(onRequestReady)
        , parser(
              this,
              [this](void) -> void {
                  VLOG(3) << "++ BEGIN:";

                  requestContexts.emplace_back(new RequestContext(this));
              },
              [&requestContexts = this->requestContexts](const std::string& method,
                                                         const std::string& url,
                                                         const std::string& httpVersion,
                                                         int httpMajor,
                                                         int httpMinor,
                                                         std::map<std::string, std::string>& queries) -> void {
                  VLOG(3) << "++ Request: " << method << " " << url << " " << httpVersion;

                  Request& request = requestContexts.back()->request;

                  request.method = method;
                  request.url = url;
                  request.queries = std::move(queries);
                  request.httpVersion = httpVersion;
                  request.httpMajor = httpMajor;
                  request.httpMinor = httpMinor;

                  VLOG(3) << "++ Queries:";
                  for (auto [query, value] : request.queries) {
                      VLOG(3) << "     " << query << ": " << value;
                  }
              },
              [&requestContexts = this->requestContexts](std::map<std::string, std::string>& header,
                                                         std::map<std::string, std::string>& cookies) -> void {
                  Request& request = requestContexts.back()->request;

                  request.headers = std::move(header);

                  VLOG(3) << "++ Headers:";
                  for (auto [field, value] : request.headers) {
                      if (field == "connection" && httputils::ci_contains(value, "close")) {
                          request.connectionState = ConnectionState::Close;
                      } else if (field == "connection" && httputils::ci_contains(value, "keep-alive")) {
                          request.connectionState = ConnectionState::Keep;
                      }
                      VLOG(3) << "     " << field << ": " << value;
                  }

                  request.cookies = std::move(cookies);

                  VLOG(3) << "++ Cookie";
                  for (auto [cookie, value] : request.cookies) {
                      VLOG(3) << "     " << cookie << ": " << value;
                  }
              },
              [&requestContexts = this->requestContexts](std::vector<uint8_t>& content) -> void {
                  VLOG(3) << "++ Content: ";

                  Request& request = requestContexts.back()->request;

                  request.body = std::move(content);
              },
              [this]() -> void {
                  VLOG(3) << "++ Parsed ++";

                  RequestContext* requestContext = requestContexts.back();

                  requestContext->ready = true;

                  requestParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(3) << "++ Error: " << status << " : " << reason;

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

        if (currentRequestContext) {
            currentRequestContext->socketContextGone();
        }
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceiveFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::requestParsed() {
        if (!requestInProgress) {
            currentRequestContext = requestContexts.front();
            requestContexts.pop_front();

            requestInProgress = true;
            if (currentRequestContext->status == 0) {
                if (((currentRequestContext->request.connectionState == ConnectionState::Close) ||
                     (currentRequestContext->request.httpMajor == 0 && currentRequestContext->request.httpMinor == 9) ||
                     (currentRequestContext->request.httpMajor == 1 && currentRequestContext->request.httpMinor == 0 &&
                      currentRequestContext->request.connectionState != ConnectionState::Keep) ||
                     (currentRequestContext->request.httpMajor == 1 && currentRequestContext->request.httpMinor == 1 &&
                      currentRequestContext->request.connectionState == ConnectionState::Close))) {
                    currentRequestContext->response.set("Connection", "close");
                } else {
                    currentRequestContext->response.set("Connection", "keep-alive");
                }

                onRequestReady(currentRequestContext->request, currentRequestContext->response);
            } else {
                currentRequestContext->response.status(currentRequestContext->status).send(currentRequestContext->reason);
                reset();
                shutdownWrite(true);
                // close();
            }
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted() {
        // if 0.9 => terminate
        // if 1.0 && (request != Keep || contentLength = -1) => terminate
        // if 1.1 && (request == Close || contentLength = -1) => terminate
        // if (request == Close) => terminate

        if ((currentRequestContext->request.httpMajor == 0 && currentRequestContext->request.httpMinor == 9) ||
            (currentRequestContext->request.httpMajor == 1 && currentRequestContext->request.httpMinor == 0 &&
             currentRequestContext->request.connectionState != ConnectionState::Keep) ||
            (currentRequestContext->request.httpMajor == 1 && currentRequestContext->request.httpMinor == 1 &&
             currentRequestContext->request.connectionState == ConnectionState::Close) ||
            (currentRequestContext->response.connectionState == ConnectionState::Close)) {
            reset();
            shutdownWrite();
        } else {
            reset();

            if (!requestContexts.empty() && requestContexts.front()->ready) {
                requestParsed();
            }
        }
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::reset() {
        requestInProgress = false;
        delete currentRequestContext;
        currentRequestContext = nullptr;
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::SocketContext::onConnected() {
        VLOG(0) << "HTTP connected";
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        VLOG(0) << "HTTP disconnected";
    }

} // namespace web::http::server
