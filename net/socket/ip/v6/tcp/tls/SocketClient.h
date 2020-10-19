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

#ifndef NET_SOCKET_IPV6_TCP_TLS_SOCKETCLIENT_H
#define NET_SOCKET_IPV6_TCP_TLS_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "socket/ip/v6/tcp/Socket.h"
#include "socket/sock_stream/tls/SocketClient.h"

namespace net::socket::ip::v6::tcp::tls {

    class SocketClient : public net::socket::stream::tls::SocketClient<net::socket::ip::v6::tcp::Socket> {
    public:
        using net::socket::stream::tls::SocketClient<net::socket::ip::v6::tcp::Socket>::SocketClient;
    };

} // namespace net::socket::ip::v6::tcp::tls

#endif // NET_SOCKET_IPV4_TCP_TLS_SOCKETCLIENT_H
