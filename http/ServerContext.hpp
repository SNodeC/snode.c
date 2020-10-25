/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "ServerContext.h"
#include "http_utils.h"
#include "socket/sock_stream/SocketConnectionBase.h"

namespace http {

    template <typename Request, typename Response>
    ServerContext<Request, Response>::ServerContext(SocketConnection* socketConnection,
                                                    const std::function<void(Request& req, Response& res)>& onRequestReady,
                                                    const std::function<void(Request& req, Response& res)>& onRequestCompleted)
        : socketConnection(socketConnection)
        , response(this)
        , onRequestCompleted(onRequestCompleted)
        , parser(
              [this](const std::string& method,
                     const std::string& url,
                     const std::string& httpVersion,
                     const std::map<std::string, std::string>& queries) -> void {
                  VLOG(1) << "++ Request: " << method << " " << url << " " << httpVersion;
                  request.method = method;
                  request.url = url;
                  request.queries = &queries;
                  request.httpVersion = httpVersion;
              },
              [this](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
                  VLOG(1) << "++ Header:";
                  request.headers = &header;
                  VLOG(1) << "++ Cookies";
                  request.cookies = &cookies;

                  for (auto [field, value] : header) {
                      if (field == "connection" && value == "keep-alive") {
                          request.keepAlive = true;
                      }
                  }
              },
              [this](char* content, size_t contentLength) -> void {
                  VLOG(1) << "++ Content: " << contentLength;
                  request.body = content;
                  request.contentLength = contentLength;
              },
              [this, onRequestReady]() -> void {
                  VLOG(1) << "++ Parsed ++";
                  requestInProgress = true;
                  request.extend();
                  onRequestReady(request, response);
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(1) << "++ Error: " << status << " : " << reason;
                  response.status(status).send(reason);
                  terminateConnection();
              }) {
        parser.reset();
        request.reset();
        response.reset();
    }

    template <typename Request, typename Response>
    ServerContext<Request, Response>::~ServerContext() {
        if (requestInProgress) {
            onRequestCompleted(request, response);
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::receiveRequestData(const char* junk, size_t junkLen) {
        if (!requestInProgress) {
            parser.parse(junk, junkLen);
        } else {
            terminateConnection();
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::onReadError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection read: " << errnum;
            responseCompleted();
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::sendResponseData(const char* buf, size_t len) {
        socketConnection->enqueue(buf, len);
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::onWriteError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write: " << errnum;
            responseCompleted();
        }
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::responseCompleted() {
        onRequestCompleted(request, response);

        if (!request.keepAlive || !response.keepAlive) {
            terminateConnection();
        }

        parser.reset();
        request.reset();
        response.reset();

        requestInProgress = false;
    }

    template <typename Request, typename Response>
    void ServerContext<Request, Response>::terminateConnection() {
        if (!connectionTerminated) {
            socketConnection->close();
            connectionTerminated = true;
        }
    }

} // namespace http
