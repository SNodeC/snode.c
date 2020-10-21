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

#ifndef TLS_CLIENT_H
#define TLS_CLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "../Client.h"
#include "socket/ip/tcp/ipv4/tls/SocketClient.h"
#include "socket/ip/tcp/ipv6/tls/SocketClient.h"

namespace http::tls {

    class Client : public http::Client<net::socket::ip::tcp::ipv4::tls::SocketClient> {
    public:
        using SocketClient = net::socket::ip::tcp::ipv4::tls::SocketClient;
        using SocketConnector = typename SocketClient::SocketConnector;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        using http::Client<SocketClient>::Client;
    };

    class Client6 : public http::Client<net::socket::ip::tcp::ipv6::tls::SocketClient> {
    public:
        using SocketClient = net::socket::ip::tcp::ipv6::tls::SocketClient;
        using SocketConnector = typename SocketClient::SocketConnector;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        using http::Client<SocketClient>::Client;
    };

} // namespace http::tls

#endif // TLS_CLIENT_H
