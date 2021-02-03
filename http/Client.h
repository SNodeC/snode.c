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

#ifndef CLIENT_H
#define CLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "ClientContext.h"
#include "Logger.h"

namespace http {

    template <typename SocketClientT>
    class Client {
    public:
        using SocketClient = SocketClientT;
        using SocketConnection = typename SocketClient::SocketConnection;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        Client(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(ServerRequest&)>& onRequestBegin,
               const std::function<void(ServerResponse&)>& onResponse,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : socketClient(
                  [onResponse, onResponseError](SocketConnection* socketConnection) -> void { // onConstruct
                      ClientContext* clientContext = new ClientContext(socketConnection, onResponse, onResponseError);
                      socketConnection->template setContext<ClientContext*>(clientContext);
                  },
                  [](SocketConnection* socketConnection) -> void { // onDestruct
                      socketConnection->template getContext<ClientContext*>([](ClientContext* clientContext) -> void {
                          delete clientContext;
                      });
                  },
                  [onConnect, onRequestBegin](SocketConnection* socketConnection) -> void { // onConnect
                      onConnect(socketConnection);

                      socketConnection->template getContext<ClientContext*>(
                          [&socketConnection, &onRequestBegin](ClientContext* clientContext) -> void {
                              clientContext->getServerRequest().setHost(socketConnection->getRemoteAddress().host());
                              onRequestBegin(clientContext->getServerRequest());
                          });
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  [](SocketConnection* socketConnection, const char* junk, std::size_t junkSize) -> void { // onRead
                      socketConnection->template getContext<ClientContext*>([junk, junkSize](ClientContext* clientContext) -> void {
                          clientContext->receiveResponseData(junk, junkSize);
                      });
                  },
                  [](SocketConnection* socketConnection, int errnum) -> void { // onReadError
                      if (errnum != 0) {
                          PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host() << " (" << errnum << ")";
                      } else {
                          VLOG(0) << "Server: EOF";
                      }
                  },
                  [](SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                      if (errnum != 0) {
                          PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host() << " (" << errnum << ")";
                      } else {
                          VLOG(0) << "Server: EOF";
                      }
                  },
                  options) {
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(int err)>& onError) const {
            errno = 0;

            socketClient.connect(remoteAddress, onError);
        }

        void connect(const std::string& ipOrHostname, unsigned short port, const std::function<void(int err)>& onError) {
            SocketAddress remoteAddress(ipOrHostname, port);

            connect(remoteAddress, onError);
        }

    protected:
        SocketClient socketClient;
    };

} // namespace http

#endif // CLIENT_H
