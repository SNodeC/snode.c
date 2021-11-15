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

#ifndef WEB_HTTP_SERVER_LEGACY_SERVER_H
#define WEB_HTTP_SERVER_LEGACY_SERVER_H

#include "net/socket/bluetooth/rfcomm/legacy/SocketServer.h"
#include "net/socket/ip/tcp/ipv4/legacy/SocketServer.h"
#include "net/socket/ip/tcp/ipv6/legacy/SocketServer.h"
#include "web/http/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server::legacy {

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class Server : public web::http::server::Server<net::socket::ip::tcp::ipv4::legacy::SocketServer, Request, Response> {
        using web::http::server::Server<net::socket::ip::tcp::ipv4::legacy::SocketServer, Request, Response>::Server;

    public:
        using SocketServer = net::socket::ip::tcp::ipv4::legacy::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

        using SocketAddress = typename SocketServer::SocketAddress;

        using web::http::server::Server<net::socket::ip::tcp::ipv4::legacy::SocketServer, Request, Response>::listen;

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(port, 5, onError);
        }

        void listen(const std::string& ipOrHostname, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, 5, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, port, 5, onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class Server6 : public web::http::server::Server<net::socket::ip::tcp ::ipv6::legacy::SocketServer, Request, Response> {
        using web::http::server::Server<net::socket::ip::tcp::ipv6::legacy::SocketServer, Request, Response>::Server;

    public:
        using SocketServer = net::socket::ip::tcp::ipv6::legacy::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

        using SocketAddress = typename SocketServer::SocketAddress;

        using web::http::server::Server<net::socket::ip::tcp::ipv6::legacy::SocketServer, Request, Response>::listen;

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(port, 5, onError);
        }

        void listen(const std::string& ipOrHostname, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, 5, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, port, 5, onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class ServerRfComm : public web::http::server::Server<net::socket::bluetooth::rfcomm::legacy::SocketServer, Request, Response> {
        using web::http::server::Server<net::socket::bluetooth::rfcomm::legacy::SocketServer, Request, Response>::Server;

    public:
        using SocketServer =
            net::socket::bluetooth::rfcomm::legacy::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

        using SocketAddress = typename SocketServer::SocketAddress;

        using web::http::server::Server<net::socket::bluetooth::rfcomm::legacy::SocketServer, Request, Response>::listen;

        void listen(uint8_t channel, const std::function<void(int)>& onError) {
            SocketServer::listen(channel, 5, onError);
        }

        void listen(const std::string& address, const std::function<void(int)>& onError) {
            SocketServer::listen(address, 5, onError);
        }

        void listen(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            SocketServer::listen(address, channel, 5, onError);
        }
    };

} // namespace web::http::server::legacy

#endif // WEB_HTTP_SERVER_LEGACY_SERVER_H
