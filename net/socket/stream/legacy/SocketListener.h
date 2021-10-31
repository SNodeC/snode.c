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

#ifndef NET_SOCKET_STREAM_LEGACY_SOCKETLISTENER_H
#define NET_SOCKET_STREAM_LEGACY_SOCKETLISTENER_H

#include "net/socket/stream/SocketListener.h"
#include "net/socket/stream/legacy/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketListener : public net::socket::stream::SocketListener<net::socket::stream::legacy::SocketConnection<SocketT>> {
    public:
        using SocketConnection = net::socket::stream::legacy::SocketConnection<SocketT>;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        SocketListener(const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                       const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : net::socket::stream::SocketListener<SocketConnection>(
                  socketContextFactory,
                  onConnect,
                  [onConnected](SocketConnection* socketConnection) -> void {
                      socketConnection->SocketConnection::SocketReader::resume();
                      onConnected(socketConnection);
                  },
                  onDisconnect,
                  options) {
        }
    };

} // namespace net::socket::stream::legacy

#endif // NET_SOCKET_STREAM_LEGACY_SOCKETLISTENER_H
