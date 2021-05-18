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

#ifndef NET_SOCKET_STREAM_SOCKETCLIENT_H
#define NET_SOCKET_STREAM_SOCKETCLIENT_H

#include "net/socket/stream/SocketConnector.h"
#include "net/socket/stream/SocketProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketConnectorT>
    class SocketClient {
    public:
        using SocketConnector = SocketConnectorT;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketClient(const std::shared_ptr<const SocketProtocolFactory> socketProtocolFactory,
                     const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onConnected,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : socketProtocolFactory(socketProtocolFactory)
            , onConnect(onConnect)
            , onConnected(onConnected)
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
            errno = 0;

            SocketConnector* socketConnector = new SocketConnector(
                socketProtocolFactory, onConnect, onConnected, onDisconnect, onRead, onReadError, onWriteError, options);

            socketConnector->connect(remoteAddress, bindAddress, onError);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(int err)>& onError) const {
            connect(remoteAddress, SocketAddress(), onError);
        }

    private:
        std::shared_ptr<const SocketProtocolFactory> socketProtocolFactory = nullptr;

        std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onConnected;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCLIENT_H
