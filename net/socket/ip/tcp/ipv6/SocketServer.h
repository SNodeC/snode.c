/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_IP_TCP_IPV6_SOCKETSERVER_H
#define NET_SOCKET_IP_TCP_IPV6_SOCKETSERVER_H

#include "net/socket/ip/tcp/SocketServer.h"
#include "net/socket/ip/tcp/ipv6/Socket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

// net::socket::ip::tcp::SocketServer<net::socket::stream::tls::SocketServer<ipv6::Socket, SocketContextFactoryT>>

namespace net::socket::ip::tcp::ipv6 {

    template <template <typename SocketT, typename SocketContextFactoryT> typename ConcreteSocketServer, typename SocketContextFactoryT>
    class SocketServer : public net::socket::ip::tcp::SocketServer<ConcreteSocketServer<ipv6::Socket, SocketContextFactoryT>> {
        using net::socket::ip::tcp::SocketServer<ConcreteSocketServer<ipv6::Socket, SocketContextFactoryT>>::SocketServer;
    };

} // namespace net::socket::ip::tcp::ipv6

#endif // NET_SOCKET_IP_TCP_IPV6_SOCKETSERVER_H
