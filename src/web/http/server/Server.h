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

#ifndef WEB_HTTP_SERVER_SERVERT_H
#define WEB_HTTP_SERVER_SERVERT_H

#include "web/http/server/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>        // IWYU pragma: export
#include <functional> // IWYU pragma: export
#include <map>        // IWYU pragma: export
#include <string>     // IWYU pragma: export

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define LISTEN_BACKLOG 512

namespace web::http::server {

    template <template <typename SocketContextFactoryT> typename SocketServerT, typename RequestT, typename ResponseT>
    class Server : public SocketServerT<web::http::server::SocketContextFactory<RequestT, ResponseT>> {
    public:
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using Super = SocketServerT<web::http::server::SocketContextFactory<Request, Response>>; // this makes it an HTTP server

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

        Server(const std::string& name,
               const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&, Response&)>& onRequestReady,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : Super(
                  name,
                  [onConnect](SocketConnection* socketConnection) -> void { // OnConnect
                      onConnect(socketConnection);
                  },
                  [onConnected](SocketConnection* socketConnection) -> void { // onConnected.
                      onConnected(socketConnection);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  options) {
            Super::getSocketContextFactory()->setOnRequestReady(onRequestReady);
        }

        Server(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&, Response&)>& onRequestReady,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : Server("", onConnect, onConnected, onRequestReady, onDisconnect, options) {
        }

        using Super::listen;

        void listen(const SocketAddress& socketAddress, const std::function<void(int)>& onError) {
            Super::listen(socketAddress, LISTEN_BACKLOG, onError);
        }
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SERVERT_H
