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

#ifndef WEB_HTTP_CLIENT_CLIENT_H
#define WEB_HTTP_CLIENT_CLIENT_H

#include "web/http/client/SocketContext.hpp"
#include "web/http/client/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    template <template <typename SocketContextFactoryT> typename SocketClientT, typename RequestT, typename ResponseT>
    class Client {
    public:
        using Request = RequestT;
        using Response = ResponseT;
        using SocketClient = SocketClientT<SocketContextFactory<Request, Response>>; // this makes it an HTTP client;
        using SocketConnection = typename SocketClient::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

        Client(const std::function<void(const Client::SocketAddress&, const Client::SocketAddress&)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&)>& onRequestBegin,
               const std::function<void(Request&, Response&)>& onResponse,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : socketClient(
                  [onConnect](const Client::SocketAddress& localAddress,
                              const Client::SocketAddress& remoteAddress) -> void { // onConnect
                      onConnect(localAddress, remoteAddress);
                  },
                  [onConnected, onRequestBegin](SocketConnection* socketConnection) -> void { // onConnected
                      Request& request = static_cast<SocketContext*>(socketConnection->getSocketContext())->getRequest();

                      request.setHost(socketConnection->getRemoteAddress().host() + ":" +
                                      std::to_string(socketConnection->getRemoteAddress().port()));
                      onConnected(socketConnection);

                      onRequestBegin(request);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  options) {
            socketClient.getSocketContextFactory()->setOnResponse(onResponse);
            socketClient.getSocketContextFactory()->setOnRequestError(onResponseError);
        }

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            socketClient.connect(SocketAddress(ipOrHostname, port), onError);
        }

        void onConnect(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect) {
            socketClient.onConnect(onConnect);
        }

        void onConnected(const std::function<void(SocketConnection*)>& onConnected) {
            socketClient.onConnected(onConnected);
        }

        void onDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            socketClient.onDisconnect(onDisconnect);
        }

    protected:
        SocketClient socketClient;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_CLIENT_H
