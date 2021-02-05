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

#ifndef HTTP_SERVER_TLS_SERVER_H
#define HTTP_SERVER_TLS_SERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "server/Server.h"
#include "socket/ip/tcp/ipv4/tls/SocketServer.h"
#include "socket/ip/tcp/ipv6/tls/SocketServer.h"

namespace http::server::tls {

    template <typename Request = http::server::Request, typename Response = http::server::Response>
    class Server : public http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response> {
    public:
        using http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response>::Server;
    };

    Server(const std::function<void(net::socket::ip::tcp::ipv4::tls::SocketServer::SocketConnection*)>& onConnect,
           const std::function<void(Request& req, Response& res)>& onRequestReady,
           const std::function<void(net::socket::ip::tcp::ipv4::tls::SocketServer::SocketConnection*)>& onDisconnect,
           const std::map<std::string, std::any>& options = {{}})
        ->Server<http::server::Request, http::server::Response>;

    template <typename Request = http::server::Request, typename Response = http::server::Response>
    class Server6 : public http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response> {
    public:
        using http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response>::Server;
    };

    Server6(const std::function<void(net::socket::ip::tcp::ipv6::tls::SocketServer::SocketConnection*)>& onConnect,
            const std::function<void(Request& req, Response& res)>& onRequestReady,
            const std::function<void(net::socket::ip::tcp::ipv6::tls::SocketServer::SocketConnection*)>& onDisconnect,
            const std::map<std::string, std::any>& options = {{}})
        ->Server6<http::server::Request, http::server::Response>;

} // namespace http::server::tls

#endif // HTTP_SERVER_TLS_SERVER_H
