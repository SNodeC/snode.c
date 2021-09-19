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

#ifndef WEB_WS_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_WS_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H

#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SocketContext.h"

namespace web::websocket::server {
    class SubProtocolFactory;
} // namespace web::websocket::server

namespace net::socket::stream {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SocketContextUpgradeFactory : public web::http::server::SocketContextUpgradeFactory {
    public:
        SocketContextUpgradeFactory() = default;

        ~SocketContextUpgradeFactory();

        static void attach(SubProtocolFactory* subProtocolFactory);

        std::string name() override;

        Role role() override;

    private:
        SocketContext* create(net::socket::stream::SocketConnection* socketConnection) const override;
    };

} // namespace web::websocket::server

#endif // WEB_WS_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
