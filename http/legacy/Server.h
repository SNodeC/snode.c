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

#ifndef LEGACY_SERVER_H
#define LEGACY_SERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "../Server.h"
#include "socket/ip/tcp/ipv4/legacy/SocketServer.h"
#include "socket/ip/tcp/ipv6/legacy/SocketServer.h"

namespace http::legacy {

    template <typename Request = http::Request, typename Response = http::Response>
    class Server : public http::Server<net::socket::ip::tcp::ipv4::legacy::SocketServer, Request, Response> {
    public:
        using SocketServer = net::socket::ip::tcp::ipv4::legacy::SocketServer;
        using SocketListener = typename SocketServer::SocketListener;
        using SocketConnection = typename SocketListener::SocketConnection;
        using Socket = typename SocketConnection::Socket;

        using http::Server<net::socket::ip::tcp::ipv4::legacy::SocketServer, Request, Response>::Server;
    };

    template <typename Request = http::Request, typename Response = http::Response>
    class Server6 : public http::Server<net::socket::ip::tcp ::ipv6::legacy::SocketServer, Request, Response> {
    public:
        using SocketServer = net::socket::ip::tcp::ipv6::legacy::SocketServer;
        using SocketListener = typename SocketServer::SocketListener;
        using SocketConnection = typename SocketListener::SocketConnection;
        using Socket = typename SocketConnection::Socket;

        using http::Server<net::socket::ip::tcp::ipv6::legacy::SocketServer, Request, Response>::Server;
    };

} // namespace http::legacy

#endif // LEGACY_SERVER_H
