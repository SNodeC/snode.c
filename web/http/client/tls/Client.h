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

#ifndef WEB_HTTP_CLIENT_TLS_CLIENT_H
#define WEB_HTTP_CLIENT_TLS_CLIENT_H

#include "net/socket/bluetooth/rfcomm/tls/SocketClient.h"
#include "net/socket/ip/tcp/ipv4/tls/SocketClient.h"
#include "net/socket/ip/tcp/ipv6/tls/SocketClient.h"
#include "web/http/client/Client.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client::tls {

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client : public web::http::client::Client<net::socket::ip::tcp::ipv4::tls::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::ip::tcp::ipv4::tls::SocketClient, Request, Response>::Client;

    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv4::tls::SocketClient, Request, Response>::socketClient;

        using web::http::client::Client<net::socket::ip::tcp::ipv4::tls::SocketClient, Request, Response>::connect;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, bindIpOrHostname, onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, bindIpOrHostname, bindPort, onError);
        }
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client6 : public web::http::client::Client<net::socket::ip::tcp::ipv6::tls::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::ip::tcp::ipv6::tls::SocketClient, Request, Response>::Client;

    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv6::tls::SocketClient, Request, Response>::socketClient;

        using web::http::client::Client<net::socket::ip::tcp::ipv6::tls::SocketClient, Request, Response>::connect;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, bindIpOrHostname, onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            socketClient.connect(ipOrHostname, port, bindIpOrHostname, bindPort, onError);
        }
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class ClientRfComm : public web::http::client::Client<net::socket::bluetooth::rfcomm::tls::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::bluetooth::rfcomm::tls::SocketClient, Request, Response>::Client;

    public:
        using web::http::client::Client<net::socket::bluetooth::rfcomm::tls::SocketClient, Request, Response>::socketClient;

        using web::http::client::Client<net::socket::bluetooth::rfcomm::tls::SocketClient, Request, Response>::connect;

        void connect(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            socketClient.connect(address, channel, onError);
        }

        void connect(const std::string& address, uint8_t channel, const std::string& bindAddress, const std::function<void(int)>& onError) {
            socketClient.connect(address, channel, bindAddress, onError);
        }

        void connect(const std::string& address,
                     uint8_t channel,
                     const std::string& bindAddress,
                     uint8_t bindChannel,
                     const std::function<void(int)>& onError) {
            socketClient.connect(address, channel, bindAddress, bindChannel, onError);
        }
    };

} // namespace web::http::client::tls

#endif // WEB_HTTP_CLIENT_TLS_CLIENT_H
