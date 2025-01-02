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

#ifndef NET_PHY_DGRAM_PEERSOCKET_H
#define NET_PHY_DGRAM_PEERSOCKET_H

#include "net/phy/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::phy::dgram {

    template <typename SocketAddressT>
    class PeerSocket : public net::phy::PhysicalSocket<SocketAddressT> {
    protected:
        using Super = net::phy::PhysicalSocket<SocketAddressT>;

    public:
        using Super::Super;

        using SocketAddress = SocketAddressT;
    };

} // namespace net::phy::dgram

#endif // NET_PHY_DGRAM_PEERSOCKET_H
