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

#include "http/client/ClientContext.h"
#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::client {
    class CookieOptions;

    template <typename Request, typename Response>
    ClientContext<Request, Response>::ClientContext(SocketConnection* socketConnection,
                                                    const std::function<void(Response&)>& onResponse,
                                                    const std::function<void(int status, const std::string& reason)>& onError)
        : socketConnection(socketConnection)
        , request(this)
        , parser(
              [](void) -> void {
              },
              [&response =
                   this->response](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
                  response.httpVersion = httpVersion;
                  response.statusCode = statusCode;
                  response.reason = reason;
              },
              [&response = this->response](const std::map<std::string, std::string>& headers,
                                           const std::map<std::string, http::CookieOptions>& cookies) -> void {
                  response.headers = &headers;
                  response.cookies = &cookies;
              },
              [&response = this->response](char* content, std::size_t contentLength) -> void {
                  response.body = content;
                  response.contentLength = contentLength;
              },
              [&response = this->response, &request = this->request, onResponse](http::client::ResponseParser& parser) -> void {
                  onResponse(response);
                  parser.reset();
                  request.reset();
                  response.reset();
              },
              [onError](int status, const std::string& reason) -> void {
                  onError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    void ClientContext<Request, Response>::requestCompleted() {
    }

    template <typename Request, typename Response>
    void ClientContext<Request, Response>::receiveResponseData(const char* junk, std::size_t junkLen) {
        parser.parse(junk, junkLen);
    }

    template <typename Request, typename Response>
    void ClientContext<Request, Response>::sendRequestData(const char* junk, std::size_t junkLen) {
        socketConnection->enqueue(junk, junkLen);
    }

    template <typename Request, typename Response>
    Request& ClientContext<Request, Response>::getRequest() {
        return request;
    }

    template <typename Request, typename Response>
    void ClientContext<Request, Response>::terminateConnection() {
        socketConnection->close();
    }

} // namespace http::client
