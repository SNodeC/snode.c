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

#ifndef WEB_HTTP_SERVER_TLS_SERVER_H
#define WEB_HTTP_SERVER_TLS_SERVER_H

#include "net/socket/bluetooth/rfcomm/tls/SocketServer.h"
#include "net/socket/ip/tcp/ipv4/tls/SocketServer.h"
#include "net/socket/ip/tcp/ipv6/tls/SocketServer.h"
#include "web/http/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server::tls {

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class Server : public web::http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response> {
    public:
        using web::http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response>::Server;
        using SocketAddress =
            typename web::http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response>::SocketAddress;

        using web::http::server::Server<net::socket::ip::tcp::ipv4::tls::SocketServer, Request, Response>::listen;

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            listen(SocketAddress(port), onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            listen(SocketAddress(ipOrHostname, port), onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class Server6 : public web::http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response> {
    public:
        using web::http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response>::Server;
        using SocketAddress =
            typename web::http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response>::SocketAddress;

        using web::http::server::Server<net::socket::ip::tcp::ipv6::tls::SocketServer, Request, Response>::listen;

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            listen(SocketAddress(port), onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            listen(SocketAddress(ipOrHostname, port), onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class ServerRfComm : public web::http::server::Server<net::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response> {
    public:
        using web::http::server::Server<net::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response>::Server;
        using SocketAddress =
            typename web::http::server::Server<net::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response>::SocketAddress;

        using web::http::server::Server<net::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response>::listen;

        void listen(uint8_t channel, const std::function<void(int)>& onError) {
            listen(SocketAddress(channel), onError);
        }

        void listen(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            listen(SocketAddress(address, channel), onError);
        }
    };

} // namespace web::http::server::tls

#endif // WEB_HTTP_SERVER_TLS_SERVER_H
