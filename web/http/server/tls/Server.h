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

#include "net/ipv4/stream/tls/SocketServer.h"   // IWYU pragma: export
#include "net/ipv6/stream/tls/SocketServer.h"   // IWYU pragma: export
#include "net/rfcomm/stream/tls/SocketServer.h" // IWYU pragma: export
#include "web/http/server/Server.h"             // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server::tls {

    template <typename Request, typename Response>
    class Server : public web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv4::SocketServer, Request, Response> {
        using web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv4::SocketServer, Request, Response>::Server;

    protected:
        using SocketServer =
            core::socket::ip::transport::tcp::tls::ipv4::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

    public:
        using web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv4::SocketServer, Request, Response>::listen;

        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& options) {
            SocketServer::addSniCert(domain, options);
        }

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(port, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& ipOrHostname, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, port, LISTEN_BACKLOG, onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class Server6 : public web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv6::SocketServer, Request, Response> {
        using web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv6::SocketServer, Request, Response>::Server;

    protected:
        using SocketServer =
            core::socket::ip::transport::tcp::tls::ipv6::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

    public:
        using web::http::server::Server<core::socket::ip::transport::tcp::tls::ipv6::SocketServer, Request, Response>::listen;

        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& options) {
            SocketServer::addSniCert(domain, options);
        }

        void listen(uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(port, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& ipOrHostname, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            SocketServer::listen(ipOrHostname, port, LISTEN_BACKLOG, onError);
        }
    };

    template <typename Request = web::http::server::Request, typename Response = web::http::server::Response>
    class ServerRfComm : public web::http::server::Server<core::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response> {
        using web::http::server::Server<core::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response>::Server;

    public:
        using SocketServer = core::socket::bluetooth::rfcomm::tls::SocketServer<web::http::server::SocketContextFactory<Request, Response>>;

        using SocketAddress = typename SocketServer::SocketAddress;

        using web::http::server::Server<core::socket::bluetooth::rfcomm::tls::SocketServer, Request, Response>::listen;

        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& options) {
            SocketServer::addSniCert(domain, options);
        }

        void listen(uint8_t channel, const std::function<void(int)>& onError) {
            SocketServer::listen(channel, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& address, const std::function<void(int)>& onError) {
            SocketServer::listen(address, LISTEN_BACKLOG, onError);
        }

        void listen(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            SocketServer::listen(address, channel, LISTEN_BACKLOG, onError);
        }
    };

} // namespace web::http::server::tls

#endif // WEB_HTTP_SERVER_TLS_SERVER_H
