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

#ifndef NET_SOCKETADDRESS_H
#define NET_SOCKETADDRESS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/SocketAddress.h" // IWYU pragma: export
#include "core/system/socket.h"        // IWYU pragma: export

#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SockAddrT>
    class SocketAddress : public core::socket::SocketAddress {
    public:
        using SockAddr = SockAddrT;
        using SockLen = socklen_t;

        explicit SocketAddress(sa_family_t af, SockLen sockAddrLen = sizeof(SockAddr));

        SocketAddress(const SocketAddress& socketAddress);

        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen);

        SocketAddress& operator=(const SocketAddress& socketAddress);

        const sockaddr& getSockAddr();
        const SockLen& getSockAddrLen() const;

        sa_family_t getAddressFamily() const;

    protected:
        SockAddr sockAddr{};
        SockLen sockAddrLen = 0;
    };

} // namespace net

#endif // NET_SOCKETADDRESS_H
