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

#include "net/socket/ip/tcp/ipv4/legacy/SocketClient.h"
#include "net/socket/ip/tcp/ipv6/legacy/SocketClient.h"
#include "web/http/client/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client::legacy {

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client : public web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response> {
    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv4::legacy::SocketClient, Request, Response>::Client;
    };

    Client(const std::function<void(const legacy::Client<>::SocketAddress&, const legacy::Client<>::SocketAddress&)>& onConnect,
           const std::function<void(legacy::Client<>::SocketConnection*)>& onConnected,
           const std::function<void(Request&)>& onRequestBegin,
           const std::function<void(Request&, Response&)>& onResponse,
           const std::function<void(int, const std::string&)>& onResponseError,
           const std::function<void(legacy::Client<>::SocketConnection*)>& onDisconnect,
           const std::map<std::string, std::any>& options = {{}})
        ->Client<web::http::client::Request, web::http::client::Response>;

    template <typename Request = web::http::client::Request, typename Response = web::http::client::Response>
    class Client6 : public web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response> {
    public:
        using web::http::client::Client<net::socket::ip::tcp::ipv6::legacy::SocketClient, Request, Response>::Client;
    };

    Client6(const std::function<void(const legacy::Client6<>::SocketAddress&, const legacy::Client6<>::SocketAddress&)>& onConnect,
            const std::function<void(legacy::Client6<>::SocketConnection*)>& onConnected,
            const std::function<void(Request&)>& onRequestBegin,
            const std::function<void(Request&, Response&)>& onResponse,
            const std::function<void(int, const std::string&)>& onResponseError,
            const std::function<void(legacy::Client6<>::SocketConnection*)>& onDisconnect,
            const std::map<std::string, std::any>& options = {{}})
        ->Client6<web::http::client::Request, web::http::client::Response>;

} // namespace web::http::client::legacy

#endif // WEB_HTTP_CLIENT_LEGACY_CLIENT_H
