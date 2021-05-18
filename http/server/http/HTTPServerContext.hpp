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

#include "http/http_utils.h"
#include "http/server/http/HTTPServerContext.h"
#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::server {

    template <typename Request, typename Response>
    HTTPServerContext<Request, Response>::HTTPServerContext(const std::function<void(Request& req, Response& res)>& onRequestReady)
        : onRequestReady(onRequestReady)
        , parser(
              [this](void) -> void {
                  VLOG(3) << "++ BEGIN:";

                  requestContexts.emplace_back(RequestContext(this));
              },
              [&requestContexts = this->requestContexts](const std::string& method,
                                                         const std::string& url,
                                                         const std::string& httpVersion,
                                                         int httpMajor,
                                                         int httpMinor,
                                                         const std::map<std::string, std::string>& queries) -> void {
                  VLOG(3) << "++ Request: " << method << " " << url << " " << httpVersion;

                  Request& request = requestContexts.back().request;

                  request.method = method;
                  request.url = url;
                  request.queries = &queries;
                  request.httpVersion = httpVersion;
                  request.httpMajor = httpMajor;
                  request.httpMinor = httpMinor;
              },
              [&requestContexts = this->requestContexts](const std::map<std::string, std::string>& header,
                                                         const std::map<std::string, std::string>& cookies) -> void {
                  Request& request = requestContexts.back().request;

                  VLOG(3) << "++ Header:";
                  request.headers = &header;

                  for (auto [field, value] : header) {
                      if (field == "connection" && httputils::ci_comp(value, "close")) {
                          request.connectionState = ConnectionState::Close;
                      } else if (field == "connection" && httputils::ci_comp(value, "keep-alive")) {
                          request.connectionState = ConnectionState::Keep;
                      }
                      VLOG(3) << "     " << field << ": " << value;
                  }

                  VLOG(3) << "++ Cookies";
                  request.cookies = &cookies;

                  for (auto [cookie, value] : cookies) {
                      VLOG(3) << "     " << cookie << ": " << value;
                  }
              },
              [&requestContexts = this->requestContexts](char* content, std::size_t contentLength) -> void {
                  VLOG(3) << "++ Content: " << contentLength;

                  Request& request = requestContexts.back().request;

                  request.body = content;
                  request.contentLength = contentLength;
              },
              [this]() -> void {
                  VLOG(3) << "++ Parsed ++";

                  RequestContext& requestContext = requestContexts.back();

                  requestContext.ready = true;

                  requestParsed();
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(3) << "++ Error: " << status << " : " << reason;

                  RequestContext& requestContext = requestContexts.back();

                  requestContext.status = status;
                  requestContext.reason = reason;
                  requestContext.ready = true;

                  requestParsed();
              }) {
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::parseReceivedData(const char* junk, std::size_t junkLen) {
        parser.parse(junk, junkLen);
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::onReadError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection read: " << errnum;
            reset();
        }
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::sendResponseData(const char* junk, std::size_t junkLen) {
        socketConnection->enqueue(junk, junkLen);
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::onWriteError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write: " << errnum;
            reset();
        }
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::requestParsed() {
        if (!requestInProgress) {
            RequestContext& requestContext = requestContexts.front();

            requestInProgress = true;
            if (requestContext.status == 0) {
                if (((requestContext.request.connectionState == ConnectionState::Close) ||
                     (requestContext.request.httpMajor == 0 && requestContext.request.httpMinor == 9) ||
                     (requestContext.request.httpMajor == 1 && requestContext.request.httpMinor == 0 &&
                      requestContext.request.connectionState != ConnectionState::Keep) ||
                     (requestContext.request.httpMajor == 1 && requestContext.request.httpMinor == 1 &&
                      requestContext.request.connectionState == ConnectionState::Close))) {
                    requestContext.response.set("Connection", "close");
                } else {
                    requestContext.response.set("Connection", "keep-alive");
                }

                onRequestReady(requestContext.request, requestContext.response);
            } else {
                requestContext.response.status(requestContext.status).send(requestContext.reason);
                terminateConnection();
            }
        }
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::responseCompleted() {
        RequestContext& requestContext = requestContexts.front();

        // if 0.9 => terminate
        // if 1.0 && (request != Keep || contentLength = -1) => terminate
        // if 1.1 && (request == Close || contentLength = -1) => terminate
        // if (request == Close) => terminate

        if ((requestContext.request.httpMajor == 0 && requestContext.request.httpMinor == 9) ||
            (requestContext.request.httpMajor == 1 && requestContext.request.httpMinor == 0 &&
             requestContext.request.connectionState != ConnectionState::Keep) ||
            (requestContext.request.httpMajor == 1 && requestContext.request.httpMinor == 1 &&
             requestContext.request.connectionState == ConnectionState::Close) ||
            (requestContext.response.connectionState == ConnectionState::Close)) {
            terminateConnection();
        } else {
            reset();

            requestContexts.pop_front();

            if (!requestContexts.empty() && requestContexts.front().ready) {
                RequestContext& requestContext = requestContexts.front();

                requestInProgress = true;
                if (requestContext.status == 0) {
                    onRequestReady(requestContext.request, requestContext.response);
                } else {
                    requestContext.response.status(requestContext.status).send(requestContext.reason);
                    terminateConnection();
                }
            }
        }
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::reset() {
        if (!requestContexts.empty()) {
            RequestContext& requestContext = requestContexts.front();
            requestContext.request.reset();
            requestContext.response.reset();
        }

        requestInProgress = false;
    }

    template <typename Request, typename Response>
    void HTTPServerContext<Request, Response>::terminateConnection() {
        if (!connectionTerminated) {
            socketConnection->close();
            requestContexts.clear();
            connectionTerminated = true;
        }

        reset();
    }

} // namespace http::server
