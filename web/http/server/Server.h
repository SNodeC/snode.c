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

#include "web/http/server/SocketContext.hpp"
#include "web/http/server/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::server {

    template <template <typename SocketContextFactoryT> typename SocketServerT, typename RequestT, typename ResponseT>
    class Server {
    public:
        using Request = RequestT;
        using Response = ResponseT;
        using SocketServer = SocketServerT<web::http::server::SocketContextFactory<Request, Response>>; // this makes it an HTTP server
        using SocketConnection = typename SocketServer::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

        Server(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&, Response&)>& onRequestReady,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : socketServer(
                  [onConnect](const SocketAddress& localAddress,
                              const SocketAddress& remoteAddress) -> void { // OnConnect
                      onConnect(localAddress, remoteAddress);
                  },
                  [onConnected](SocketConnection* socketConnection) -> void { // onConnected.
                      onConnected(socketConnection);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  options) {
            socketServer.getSocketContextFactory()->setOnRequestReady(onRequestReady);
        }

        void listen(const SocketAddress& socketAddress, const std::function<void(int)>& onError) {
            socketServer.listen(socketAddress, 5, onError);
        }

        void onConnect(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect) {
            socketServer.onConnect(onConnect);
        }

        void onConnected(const std::function<void(SocketConnection*)>& onConnected) {
            socketServer.onConnected(onConnected);
        }

        void onDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            socketServer.onDisconnect(onDisconnect);
        }

    protected:
        SocketServer socketServer;

        std::function<void(Request&, Response&)> onRequestReady;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SERVERT_H
