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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/legacy/SocketReader.h"
#include "core/socket/stream/legacy/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename SocketT>
    class SocketConnection
        : public core::socket::stream::SocketConnection<SocketReader<SocketT>, SocketWriter<SocketT>, typename SocketT::SocketAddress> {
    public:
        using Socket = SocketT;
        using SocketAddress = typename Socket::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<SocketContextFactory>& socketProtocolFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                         const std::function<void(SocketConnection*)>& onDisconnect)
            : SocketConnection::Descriptor(fd)
            , core::socket::stream::SocketConnection<SocketReader<Socket>, SocketWriter<Socket>, typename Socket::SocketAddress>::
                  SocketConnection(socketProtocolFactory, localAddress, remoteAddress, onConnect, [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  }) {
        }

        template <typename Socket>
        friend class SocketAcceptor;

        template <typename Socket>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
