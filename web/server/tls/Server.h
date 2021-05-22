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

#ifndef HTTP_SERVER_TLS_SERVER_H
#define HTTP_SERVER_TLS_SERVER_H

#include "net/socket/ip/tcp/ipv4/tls/SocketServer.h"
#include "net/socket/ip/tcp/ipv6/tls/SocketServer.h"
#include "web/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::server::tls {

    template <typename Request = web::server::http::Request, typename Response = web::server::http::Response>
    class Server : public web::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response> {
    public:
        using web::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response>::Server;
    };

    template <typename Request = web::server::http::Request, typename Response = web::server::http::Response>
    class Server6 : public web::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response> {
    public:
        using web::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response>::Server;
    };

} // namespace web::server::tls

#endif // HTTP_SERVER_TLS_SERVER_H
