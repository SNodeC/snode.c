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

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::legacy {

    template <typename PhysicalSocket>
    SocketConnection<PhysicalSocket>::SocketConnection(const std::string& instanceName,
                                                       PhysicalSocket&& physicalSocket,
                                                       const std::function<void(SocketConnection*)>& onDisconnect,
                                                       const SocketAddress& localPeerAddress,
                                                       const SocketAddress& remotePeerAddress,
                                                       const utils::Timeval& readTimeout,
                                                       const utils::Timeval& writeTimeout,
                                                       std::size_t readBlockSize,
                                                       std::size_t writeBlockSize,
                                                       const utils::Timeval& terminateTimeout)
        : Super(
              instanceName,
              std::move(physicalSocket),
              [onDisconnect, this]() -> void {
                  onDisconnect(this);
              },
              localPeerAddress,
              remotePeerAddress,
              readTimeout,
              writeTimeout,
              readBlockSize,
              writeBlockSize,
              terminateTimeout) {
    }

} // namespace core::socket::stream::legacy
