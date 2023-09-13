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

// clang-format off
#include "core/socket/stream/legacy/SocketConnection.h"
#include "core/socket/stream/SocketConnection.hpp" // IWYU pragma: export
// clang-format on

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::legacy {

    template <typename PhysicalSocket>
    core::socket::stream::legacy::SocketConnection<PhysicalSocket>::SocketConnection(
        PhysicalSocket& physicalSocket,
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const SocketAddress& localAddress,
        const SocketAddress& remoteAddress,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const utils::Timeval& readTimeout,
        const utils::Timeval& writeTimeout,
        std::size_t readBlockSize,
        std::size_t writeBlockSize,
        const utils::Timeval& terminateTimeout)
        : PhysicalSocket(physicalSocket)
        , Super(
              socketContextFactory,
              localAddress,
              remoteAddress,
              [onDisconnect, this]() -> void {
                  onDisconnect(this);
              },
              readTimeout,
              writeTimeout,
              readBlockSize,
              writeBlockSize,
              terminateTimeout) {
    }

} // namespace core::socket::stream::legacy
