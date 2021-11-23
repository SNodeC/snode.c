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

#ifndef WEB_WEBSOCKET_CLIENT_SOCKETCONTEXT_H
#define WEB_WEBSOCKET_CLIENT_SOCKETCONTEXT_H

#include "web/websocket/SocketContextUpgrade.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

namespace web::websocket::client {
    class SubProtocol;
} // namespace web::websocket::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    class SocketContextUpgrade
        : public web::websocket::SocketContextUpgrade<SubProtocol, web::http::client::Request, web::http::client::Response> {
    public:
        SocketContextUpgrade(
            core::socket::stream::SocketConnection* socketConnection,
            web::http::SocketContextUpgradeFactory<web::http::client::Request, web::http::client::Response>* socketContextUpgradeFactory,
            web::websocket::client::SubProtocol* subProtocol);

    protected:
        ~SocketContextUpgrade() override;

    public:
        static SocketContextUpgrade*
        create(web::http::SocketContextUpgradeFactory<web::http::client::Request, web::http::client::Response>* socketContextUpgradeFactory,
               core::socket::stream::SocketConnection* socketConnection,
               const std::string& subProtocolName);
    };

} // namespace web::websocket::client

#endif // WEB_WEBSOCKET_CLIENT_SOCKETCONTEXT_H
