/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H

#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/legacy/SocketConnection.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename PhysicalServerSocketT, typename ConfigT>
    class SocketAcceptor
        : private core::socket::stream::SocketAcceptor<PhysicalServerSocketT, ConfigT, core::socket::stream::legacy::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<PhysicalServerSocketT, ConfigT, core::socket::stream::legacy::SocketConnection>;
        using SocketAddress = typename Super::SocketAddress;
        using Config = ConfigT;

    public:
        using SocketConnection = typename Super::SocketConnection;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, int)>& onError,
                       const std::shared_ptr<Config>& config)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected](SocketConnection* socketConnection) -> void {
                      onConnected(socketConnection);
                      socketConnection->onConnected();
                  },
                  onDisconnect,
                  onError,
                  config) {
        }
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H
