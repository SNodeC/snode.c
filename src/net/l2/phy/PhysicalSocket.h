/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef NET_L2_PHY_PHYSICALSOCKET_H
#define NET_L2_PHY_PHYSICALSOCKET_H

#include "net/l2/SocketAddress.h" // IWYU pragma: export
#include "net/phy/PhysicalSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::phy {

    template <template <typename SocketAddressT> typename PhysicalPeerSocketT>
    class PhysicalSocket : public PhysicalPeerSocketT<net::l2::SocketAddress> {
    public:
        using Super = PhysicalPeerSocketT<net::l2::SocketAddress>;
        using Super::Super;

        PhysicalSocket(int type, int protocol);
        PhysicalSocket(PhysicalSocket&&) noexcept = default;

        ~PhysicalSocket() override;
    };

} // namespace net::l2::phy

extern template class net::phy::PhysicalSocket<net::l2::SocketAddress>;

#endif // NET_L2_PHY_PHYSICALSOCKET_H
