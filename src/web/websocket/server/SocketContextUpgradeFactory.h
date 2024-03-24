/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef WEB_WEBSOCKET_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_WEBSOCKET_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H

#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SocketContextUpgrade.h"

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SocketContextUpgradeFactory : public web::http::server::SocketContextUpgradeFactory {
    public:
        SocketContextUpgradeFactory() = default;

        static void link();

    private:
        std::string name() override;

        SocketContextUpgrade* create(core::socket::stream::SocketConnection* socketConnection,
                                     web::http::server::Request* request,
                                     web::http::server::Response* response) override;
    };

    extern "C" web::http::server::SocketContextUpgradeFactory* websocketServerContextUpgradeFactory();

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
