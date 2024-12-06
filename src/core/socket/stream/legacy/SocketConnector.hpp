/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

// clang-format off
#include "core/socket/stream/legacy/SocketConnector.h"
#include "core/socket/stream/SocketConnector.hpp"
// clang-format on

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::legacy {

    template <typename PhysicalClientSocket, typename Config>
    core::socket::stream::legacy::SocketConnector<PhysicalClientSocket, Config>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : Super(
              socketContextFactory,
              onConnect,
              [socketContextFactory, onConnected](SocketConnection* socketConnection) -> void {
                  socketConnection->connectSocketContext(socketContextFactory);

                  onConnected(socketConnection);
              },
              onDisconnect,
              onStatus,
              config) {
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketConnector<PhysicalSocketServer, Config>::SocketConnector(const SocketConnector& socketConnector)
        : Super::SocketConnector(socketConnector) {
    }

    template <typename PhysicalClientSocket, typename Config>
    void core::socket::stream::legacy::SocketConnector<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketConnector(*this);
    }

} // namespace core::socket::stream::legacy
