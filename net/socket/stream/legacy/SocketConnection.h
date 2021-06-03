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

#ifndef NET_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
#define NET_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H

#include "net/socket/stream/SocketConnection.h"
#include "net/socket/stream/legacy/SocketReader.h"
#include "net/socket/stream/legacy/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketConnection
        : public stream::SocketConnection<legacy::SocketReader<SocketT>, legacy::SocketWriter<SocketT>, typename SocketT::SocketAddress> {
    public:
        using Socket = SocketT;
        using SocketAddress = typename Socket::SocketAddress;

    public:
        SocketConnection(const std::shared_ptr<const SocketContextFactory>& socketProtocolFactory,
                         int fd,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                         const std::function<void(SocketConnection* socketConnection)>& onDisconnect)
            : stream::SocketConnection<legacy::SocketReader<Socket>, legacy::SocketWriter<Socket>, typename Socket::SocketAddress>::
                  SocketConnection(socketProtocolFactory, fd, localAddress, remoteAddress, onConnect, [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  }) {
        }

    private:
        std::function<void(SocketConnection* socketConnection)> onDestruct;
    };

} // namespace net::socket::stream::legacy

#endif // NET_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
