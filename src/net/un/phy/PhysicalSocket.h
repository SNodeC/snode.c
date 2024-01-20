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

#ifndef NET_UN_PHY_PHYSICALSOCKET_H
#define NET_UN_PHY_PHYSICALSOCKET_H

#include "net/phy/PhysicalSocket.h"
#include "net/un/SocketAddress.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::phy {

    template <template <typename SocketAddressT> typename PhysicalPeerSocketT>
    class PhysicalSocket : public PhysicalPeerSocketT<net::un::SocketAddress> {
    public:
        using Super = PhysicalPeerSocketT<net::un::SocketAddress>;
        using Super::Super;

        PhysicalSocket(int type, int protocol);
        PhysicalSocket(PhysicalSocket&& physicalSocket) noexcept;

        PhysicalSocket& operator=(PhysicalSocket&& physicalSocket) noexcept;

        ~PhysicalSocket() override;

        int bind(SocketAddress& bindAddress);

    private:
        int lockFd = -1;
    };

} // namespace net::un::phy

extern template class net::phy::PhysicalSocket<net::un::SocketAddress>;

#endif // NET_UN_PHY_PHYSICALSOCKET_H
