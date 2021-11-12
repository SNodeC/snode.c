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

#ifndef WEB_HTTP_CLIENT_LEGACY_CLIENT_H
#define WEB_HTTP_CLIENT_LEGACY_CLIENT_H

#include "net/socket/bluetooth/rfcomm/legacy/SocketClient.h"
#include "net/socket/ip/tcp/ipv4/legacy/SocketClient.h"
#include "net/socket/ip/tcp/ipv6/legacy/SocketClient.h"
#include "web/http/client/Client.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client::legacy {

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client : public web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response> {
    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response>::Client;
        using web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response>::connect;
        using SocketAddress =
            typename web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response>::SocketAddress;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
        }
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client6 : public web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response> {
    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response>::Client;
        using web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response>::connect;
        using SocketAddress =
            typename web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response>::SocketAddress;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
        }
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class ClientRfComm : public web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response> {
    public:
        using web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response>::Client;
        using web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response>::connect;
        using SocketAddress =
            typename web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response>::SocketAddress;

        void connect(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), onError);
        }

        void connect(const std::string& address,
                     uint8_t channel,
                     const std::string& bindAddress,
                     uint8_t bindChannel,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), SocketAddress(bindAddress, bindChannel), onError);
        }
    };

} // namespace web::http::client::legacy

#endif // WEB_HTTP_CLIENT_LEGACY_CLIENT_H
