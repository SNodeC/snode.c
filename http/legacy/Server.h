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
#include "socket/ipv4/tcp/legacy/SocketServer.h"
#include "socket/ipv6/tcp/legacy/SocketServer.h"

namespace http::legacy {

    class Server : public http::Server<net::socket::ipv4::tcp::legacy::SocketServer> {
    public:
        using SocketServer = net::socket::ipv4::tcp::legacy::SocketServer;
        using SocketListener = typename SocketServer::SocketListener;
        using SocketConnection = typename SocketListener::SocketConnection;
        using Socket = typename SocketServer::Socket;

        using http::Server<net::socket::ipv4::tcp::legacy::SocketServer>::Server;
    };

    class Server6 : public http::Server<net::socket::ipv6::tcp::legacy::SocketServer> {
    public:
        using SocketServer = net::socket::ipv6::tcp::legacy::SocketServer;
        using SocketListener = typename SocketServer::SocketListener;
        using SocketConnection = typename SocketListener::SocketConnection;
        using Socket = typename SocketServer::Socket;

        using http::Server<net::socket::ipv6::tcp::legacy::SocketServer>::Server;
    };

} // namespace http::legacy

#endif // LEGACY_SERVER_H
