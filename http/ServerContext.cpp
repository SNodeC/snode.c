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

#include "ServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "http_utils.h"
#include "socket/SocketConnectionBase.h"

namespace http {

    ServerContext::ServerContext(SocketConnection* socketConnection, const std::function<void(Request& req, Response& res)>& onRequestReady,
                                 const std::function<void(Request& req, Response& res)>& onRequestCompleted)
        : socketConnection(socketConnection)
        , response(this)
        , onRequestCompleted(onRequestCompleted)
        , parser(
              [this](const std::string& method, const std::string& url, const std::string& httpVersion,
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
              [this, onRequestReady]([[maybe_unused]] http::RequestParser& requestParser) -> void {
                  VLOG(1) << "++ Parsed ++";
                  requestInProgress = true;
                  onRequestReady(request, response);
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(1) << "++ Error: " << status << " : " << reason;
                  response.status(status).send(reason);
                  this->socketConnection->end();
              }) {
        parser.reset();
        request.reset();
        response.reset();
    }

    ServerContext::~ServerContext() {
        if (requestInProgress) {
            onRequestCompleted(request, response);
        }
    }

    void ServerContext::receiveRequestData(const char* junk, size_t junkLen) {
        if (!requestInProgress) {
            parser.parse(junk, junkLen);
        } else {
            terminateConnection();
        }
    }

    void ServerContext::onReadError(int errnum) {
        response.disable();

        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection: read";
        }
    }

    void ServerContext::sendResponseData(const char* buf, size_t len) {
        socketConnection->enqueue(buf, len);
    }

    void ServerContext::onWriteError(int errnum) {
        response.disable();

        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write";
        }
    }

    void ServerContext::responseCompleted() {
        onRequestCompleted(request, response);

        if (!request.keepAlive || !response.keepAlive) {
            terminateConnection();
        }

        parser.reset();
        request.reset();
        response.reset();

        requestInProgress = false;
    }

    void ServerContext::terminateConnection() {
        socketConnection->end();
    }

} // namespace http
