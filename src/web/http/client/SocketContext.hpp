/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/SocketAddress.h"
#include "core/socket/SocketConnection.h"
#include "web/http/client/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename Request, typename Response>
    SocketContext<Request, Response>::SocketContext(core::socket::stream::SocketConnection* socketConnection,
                                                    const std::function<void(Request&)>& onRequestBegin,
                                                    const std::function<void(Request&, Response&)>& onResponse,
                                                    const std::function<void(int, const std::string&)>& onError)
        : Super(socketConnection)
        , onRequestBegin(onRequestBegin)
        , request(this)
        , response(this)
        , parser(
              this,
              []() -> void {
              },
              [&response = this->response](std::string& httpVersion, std::string& statusCode, std::string& reason) -> void {
                  response.httpVersion = httpVersion;
                  response.statusCode = statusCode;
                  response.reason = reason;
              },
              [&response = this->response](std::map<std::string, std::string>& headers,
                                           std::map<std::string, web::http::CookieOptions>& cookies) -> void {
                  response.headers = std::move(headers);
                  response.cookies = std::move(cookies);
              },
              [&response = this->response](std::vector<uint8_t>& content) -> void {
                  response.body = std::move(content);
              },
              [onResponse, this](web::http::client::ResponseParser& parser) -> void {
                  onResponse(request, response);

                  if (response.header("connection") == "close" || request.header("connection") == "close") {
                      shutdownWrite();
                  }

                  parser.reset();
                  request.reset();
                  response.reset();
              },
              [onError, this](int status, const std::string& reason) -> void {
                  onError(status, reason);

                  shutdownWrite(true);
              }) {
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::sendToPeerCompleted() {
    }

    template <typename Request, typename Response>
    std::size_t SocketContext<Request, Response>::onReceiveFromPeer() {
        return parser.parse();
    }

    template <typename Request, typename Response>
    Request& SocketContext<Request, Response>::getRequest() {
        return request;
    }

    template <typename Request, typename Response>
    Response& SocketContext<Request, Response>::getResponse() {
        return response;
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::SocketContext::onConnected() {
        VLOG(0) << "HTTP connected";

        request.setHost(socketConnection->getRemoteAddress().toString());

        onRequestBegin(request);
    }

    template <typename Request, typename Response>
    void SocketContext<Request, Response>::onDisconnected() {
        VLOG(0) << "HTTP disconnected";
    }

} // namespace web::http::client
