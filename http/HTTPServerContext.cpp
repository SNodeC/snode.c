/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "HTTPServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "httputils.h"
#include "socket/SocketConnectionBase.h"

namespace http {

    HTTPServerContext::HTTPServerContext(net::socket::SocketConnectionBase* connectedSocket,
                                         const std::function<void(Request& req, Response& res)>& onRequestReady,
                                         const std::function<void(Request& req, Response& res)>& onResponseFinished)
        : connectedSocket(connectedSocket)
        , onRequestReady(onRequestReady)
        , onResponseFinished(onResponseFinished)
        , response(this)
        , parser(
              [this](std::string& method, std::string& originalUrl, std::string& httpVersion,
                     const std::map<std::string, std::string>& queries) -> void {
                  VLOG(1) << "++ Request: " << method << " " << originalUrl << " " << httpVersion;
                  request.method = method;
                  request.originalUrl = originalUrl;
                  request.path = httputils::str_split_last(originalUrl, '/').first;
                  request.queries = &queries;
                  if (request.path.empty()) {
                      request.path = "/";
                  }
                  request.url = httputils::str_split_last(originalUrl, '?').first;
                  request.httpVersion = httpVersion;
              },
              [this](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
                  VLOG(1) << "++ Header:";
                  request.headers = &header;
                  for (auto& [field, value] : header) {
                      if (field == "connection" && value == "keep-alive") {
                          request.keepAlive = true;
                      }
                  }
                  VLOG(1) << "++ Cookies";
                  request.cookies = &cookies;
              },
              [this](char* content, size_t contentLength) -> void {
                  VLOG(1) << "++ Content: " << contentLength;
                  request.body = content;
                  request.contentLength = contentLength;
              },
              [this](void) -> void {
                  VLOG(1) << "++ Parsed ++";
                  this->requestReady();
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(1) << "++ Error: " << status << " : " << reason;
                  response.status(status).send(reason);
                  this->connectedSocket->end();
              }) {
        parser.reset();
        request.reset();
        response.reset();
    }

    void HTTPServerContext::receiveRequestData(const char* junk, size_t junkLen) {
        if (!requestInProgress) {
            parser.parse(junk, junkLen);
        } else {
            terminateConnection();
        }
    }

    void HTTPServerContext::onReadError(int errnum) {
        response.disable();

        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection: read";
        }
    }

    void HTTPServerContext::sendResponseData(const char* buf, size_t len) {
        connectedSocket->enqueue(buf, len);
    }

    void HTTPServerContext::onWriteError(int errnum) {
        response.disable();

        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write";
        }
    }

    void HTTPServerContext::requestReady() {
        this->requestInProgress = true;

        onRequestReady(request, response);
    }

    void HTTPServerContext::responseCompleted() {
        onResponseFinished(request, response);

        if (!request.keepAlive || !response.keepAlive) {
            terminateConnection();
        }

        parser.reset();
        request.reset();
        response.reset();

        this->requestInProgress = false;
    }

    void HTTPServerContext::terminateConnection() {
        connectedSocket->end();
    }

} // namespace http
