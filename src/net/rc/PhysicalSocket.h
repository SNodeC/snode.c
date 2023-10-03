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

#ifndef NET_RC_PHYSICALSOCKET_H
#define NET_RC_PHYSICALSOCKET_H

#include "net/phy/PhysicalSocket.h" // IWYU pragma: export
#include "net/rc/SocketAddress.h"   // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc {

    template <template <typename SocketAddressT> typename PhysicalPeerSocketT>
    class PhysicalSocket : public PhysicalPeerSocketT<net::rc::SocketAddress> {
    private:
        using Super = PhysicalPeerSocketT<net::rc::SocketAddress>;

    public:
        using Super::Super;

        PhysicalSocket(int type, int protocol);
        PhysicalSocket(const PhysicalSocket&) = default;

        ~PhysicalSocket() override;
    };

} // namespace net::rc

extern template class net::phy::PhysicalSocket<net::rc::SocketAddress>;

#endif // NET_RC_PHYSICALSOCKET_H
