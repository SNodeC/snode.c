/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H
#define WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H

#include "web/websocket/SocketContextUpgrade.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace web {
    namespace http::server {
        class Request;
        class Response;
    } // namespace http::server

    namespace websocket::server {
        class SubProtocol;
    }
} // namespace web

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SocketContextUpgrade
        : public web::websocket::SocketContextUpgrade<SubProtocol, web::http::server::Request, web::http::server::Response> {
    public:
        SocketContextUpgrade(
            core::socket::SocketConnection* socketConnection,
            web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
            web::websocket::server::SubProtocol* subProtocol);

    protected:
        ~SocketContextUpgrade() override;

    public:
        static SocketContextUpgrade*
        create(web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
               core::socket::SocketConnection* socketConnection,
               const std::string& subProtocolName);
    };

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H
