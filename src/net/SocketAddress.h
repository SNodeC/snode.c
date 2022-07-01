/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/system/socket.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SockAddrT>
    class SocketAddress {
    public:
        using SockAddr = SockAddrT;

        SocketAddress();

        SocketAddress(const SocketAddress& socketAddress);

        virtual ~SocketAddress() = default;

        SocketAddress& operator=(const SocketAddress& socketAddress);

        SocketAddress& operator=(const SockAddr& sockAddr);

        sockaddr& getSockAddr();
        const sockaddr& getSockAddr() const;

        socklen_t getSockAddrLen() const;

        virtual std::string address() const = 0;
        virtual std::string toString() const = 0;

    protected:
        SockAddr sockAddr;
    };

} // namespace net

#endif // NET_SOCKETADDRESS_H
