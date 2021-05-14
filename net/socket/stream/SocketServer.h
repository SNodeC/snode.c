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

#ifndef NET_SOCKET_STREAM_SOCKETSERVERNEW_H
#define NET_SOCKET_STREAM_SOCKETSERVERNEW_H

#include "net/socket/stream/SocketListener.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <cstddef>
#include <functional>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketListenerT>
    class SocketServer {
    public:
        using SocketListener = SocketListenerT;
        using SocketConnection = typename SocketListener::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketServer(const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onConnected,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , options(options) {
        }

        SocketServer() = delete;

        virtual ~SocketServer() = default;

        void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int err)>& onError) const {
            errno = 0;

            SocketListener* socketListener =
                new SocketListener(onConnect, onConnected, onDisconnect, onRead, onReadError, onWriteError, options);

            socketListener->listen(bindAddress, backlog, onError);
        }

    private:
        std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onConnected;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETSERVERNEW_H
