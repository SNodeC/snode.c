/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_IP_TCP_IPV4_SOCKET_H
#define NET_SOCKET_IP_TCP_IPV4_SOCKET_H

#include "net/socket/Socket.h"
#include "net/socket/ip/address/ipv4/InetAddress.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::ip::tcp::ipv4 {

    class Socket : public socket::Socket<address::ipv4::InetAddress> {
    protected:
        int create(int flags) override;
    };

} // namespace net::socket::ip::tcp::ipv4

#endif // NET_SOCKET_IP_TCP_IPV4_SOCKET_H
