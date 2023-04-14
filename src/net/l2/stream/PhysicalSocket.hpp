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

#include "net/l2/PhysicalSocket.hpp"      // IWYU pragma: export
#include "net/l2/stream/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket()
        : Super(SOCK_STREAM, BTPROTO_L2CAP) {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::~PhysicalSocket() {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    void PhysicalSocket<PhysicalPeerSocket>::shutdown([[maybe_unused]] typename Super::SHUT how) {
        Super::shutdown(Super::SHUT::RDWR); // always shutdown L2CAP sockets for RDWR
    }

} // namespace net::l2::stream
