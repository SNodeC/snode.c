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
#include "web/http/client/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"

namespace web::http {
    class CokieOptions;
} // namespace web::http

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContext::SocketContext(net::socket::stream::SocketConnection* socketConnection)
        : net::socket::stream::SocketContext(socketConnection) {
    }

    template <typename Request, typename Response>
    SocketContextT<Request, Response>::SocketContextT(net::socket::stream::SocketConnection* socketConnection,
                                                      const std::function<void(Request&, Response&)>& onResponse,
                                                      const std::function<void(int, const std::string&)>& onError)
        : SocketContext(socketConnection)
        , request(this)
        , parser(
              this,
              [](void) -> void {
              },
              [&response =
                   this->response](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
                  response.httpVersion = httpVersion;
                  response.statusCode = statusCode;
                  response.reason = reason;
              },
              [&response = this->response](const std::map<std::string, std::string>& headers,
                                           const std::map<std::string, web::http::CookieOptions>& cookies) -> void {
                  response.headers = &headers;
                  response.cookies = &cookies;
              },
              [&response = this->response](char* content, std::size_t contentLength) -> void {
                  response.body = content;
                  response.contentLength = contentLength;
              },
              [&response = this->response, &request = this->request, onResponse](web::http::client::ResponseParser& parser) -> void {
                  onResponse(request, response);
                  parser.reset();
                  request.reset();
                  response.reset();
              },
              [onError](int status, const std::string& reason) -> void {
                  onError(status, reason);
              }) {
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::sendToPeerCompleted() {
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::stop() {
        parser.stop();
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::onReceiveFromPeer() {
        parser.parse();
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::onWriteError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection write: " << errnum;
        }
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::onReadError(int errnum) {
        if (errnum != 0 && errnum != ECONNRESET) {
            PLOG(ERROR) << "Connection read: " << errnum;
        }
    }

    template <typename Request, typename Response>
    Request& SocketContextT<Request, Response>::getRequest() {
        return request;
    }

    template <typename Request, typename Response>
    Response& SocketContextT<Request, Response>::getResponse() {
        return response;
    }

    template <typename Request, typename Response>
    void SocketContextT<Request, Response>::terminateConnection() {
        close();
    }

} // namespace web::http::client
