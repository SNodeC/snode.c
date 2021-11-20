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

#include "core/socket/bluetooth/rfcomm/legacy/SocketClient.h"      // IWYU pragma: export
#include "core/socket/ip/transport/tcp/legacy/ipv4/SocketClient.h" // IWYU pragma: export
#include "core/socket/ip/transport/tcp/legacy/ipv6/SocketClient.h" // IWYU pragma: export
#include "web/http/client/Client.h"                               // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client::legacy {

    template <typename Request, typename Response>
    class Client : public web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv4::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv4::SocketClient, Request, Response>::Client;

    protected:
        using SocketClient =
            net::socket::ip::transport::tcp::legacy::ipv4::SocketClient<web::http::client::SocketContextFactory<Request, Response>>;

    public:
        using web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv4::SocketClient, Request, Response>::connect;
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client6 : public web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv6::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv6::SocketClient, Request, Response>::Client;

    protected:
        using SocketClient =
            net::socket::ip::transport::tcp::legacy::ipv6::SocketClient<web::http::client::SocketContextFactory<Request, Response>>;

    public:
        using web::http::client::Client<net::socket::ip::transport::tcp::legacy::ipv6::SocketClient, Request, Response>::connect;
    };

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class ClientRfComm : public web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response> {
        using web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response>::Client;

    public:
        using SocketClient =
            net::socket::bluetooth::rfcomm::legacy::SocketClient<web::http::client::SocketContextFactory<Request, Response>>;

        using SocketAddress = typename SocketClient::SocketAddress;

        using web::http::client::Client<net::socket::bluetooth::rfcomm::legacy::SocketClient, Request, Response>::connect;
    };

} // namespace web::http::client::legacy

#endif // WEB_HTTP_CLIENT_LEGACY_CLIENT_H
