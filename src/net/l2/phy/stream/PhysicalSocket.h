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

#ifndef NET_L2_PHY_STREAM_PHYSICALSOCKET_H
#define NET_L2_PHY_STREAM_PHYSICALSOCKET_H

#include "net/l2/phy/PhysicalSocket.h" // IWYU pragma: export
#include "net/phy/stream/PhysicalSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::phy::stream {

    template <template <typename SocketAddressT> typename PhysicalPeerSocketT>
    class PhysicalSocket : public net::l2::phy::PhysicalSocket<PhysicalPeerSocketT> {
    public:
        using Super = net::l2::phy::PhysicalSocket<PhysicalPeerSocketT>;
        using Super::Super;

        PhysicalSocket();
        PhysicalSocket(PhysicalSocket&&) noexcept = default;

        ~PhysicalSocket() override;

        int shutdown(typename Super::SHUT how); // shutdown L2CAP sockets must be handled differently
    };

} // namespace net::l2::phy::stream

extern template class net::phy::stream::PhysicalSocket<net::l2::SocketAddress>;

#endif // NET_L2_PHY_STREAM_PHYSICALSOCKET_H
