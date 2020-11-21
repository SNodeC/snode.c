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

#ifndef NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H
#define NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <functional>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnector.h"

namespace net::socket::stream {

    template <typename SocketConnectorT>
    class SocketClient {
    public:
        using SocketConnector = SocketConnectorT;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketClient(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                     const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                     const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : onConstruct(onConstruct)
            , onDestruct(onDestruct)
            , onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , options(options) {
        }

        SocketClient() = delete;

        virtual ~SocketClient() = default;

        void
        connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int err)>& onError) const {
            SocketConnector* socketConnector =
                new SocketConnector(onConstruct, onDestruct, onConnect, onDisconnect, onRead, onReadError, onWriteError, this->options);

            socketConnector->connect(remoteAddress, bindAddress, onError);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(int err)>& onError) const {
            SocketAddress bindAddress;

            connect(remoteAddress, bindAddress, onError);
        }

    private:
        std::function<void(SocketConnection* socketConnection)> onConstruct;
        std::function<void(SocketConnection* socketConnection)> onDestruct;
        std::function<void(SocketConnection* socketConnection)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H
