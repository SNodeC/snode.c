/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/SocketConnectionFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocket, typename Config, typename SocketConnection>
    SocketConnectionFactory<PhysicalSocket, Config, SocketConnection>::SocketConnectionFactory(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect)
        : socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect) {
    }

    template <typename PhysicalSocket, typename Config, typename SocketConnection>
    bool SocketConnectionFactory<PhysicalSocket, Config, SocketConnection>::create(PhysicalSocket& physicalSocket,
                                                                                   const std::shared_ptr<Config>& config) {
        SocketConnection* socketConnection = nullptr;

        if (physicalSocket.isValid()) {
            typename SocketAddress::SockAddr localSockAddr;
            typename SocketAddress::SockAddr remoteSockAddr;
            socklen_t localSockAddrLen = sizeof(typename SocketAddress::SockAddr);
            socklen_t remoteSockAddrLen = sizeof(typename SocketAddress::SockAddr);

            if (physicalSocket.getSockname(localSockAddr, localSockAddrLen) == 0 &&
                physicalSocket.getPeername(remoteSockAddr, remoteSockAddrLen) == 0) {
                physicalSocket.setDontClose();

                socketConnection = new SocketConnection(physicalSocket,
                                                        socketContextFactory,
                                                        SocketAddress(localSockAddr, localSockAddrLen),
                                                        SocketAddress(remoteSockAddr, remoteSockAddrLen),
                                                        onDisconnect,
                                                        config->getReadTimeout(),
                                                        config->getWriteTimeout(),
                                                        config->getReadBlockSize(),
                                                        config->getWriteBlockSize(),
                                                        config->getTerminateTimeout());
                if (socketConnection->core::socket::stream::SocketConnection::isValid()) {
                    onConnect(socketConnection);
                    onConnected(socketConnection);
                } else {
                    LOG(ERROR) << "SocketConnectionFactory: Failed creating new SocketConnection";

                    delete socketConnection;
                    socketConnection = nullptr;
                }
            } else {
                PLOG(ERROR) << "getsockname";
            }
        }

        return socketConnection != nullptr;
    }

} // namespace core::socket::stream
