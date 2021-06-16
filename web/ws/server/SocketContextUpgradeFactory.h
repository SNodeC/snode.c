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

#ifndef WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H
#define WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H

#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/http/server/SocketContextUpgradeInterface.h"
#include "web/ws/server/SocketContext.h"
#include "web/ws/server/SubProtocolSelector.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

namespace web::ws::server {

    class SocketContextUpgradeFactory : public web::http::server::SocketContextUpgradeFactory {
    public:
        SocketContextUpgradeFactory();
        ~SocketContextUpgradeFactory();

        std::string name() override;
        ROLE role() override;

    private:
        web::ws::server::SocketContext* create(net::socket::stream::SocketConnection* socketConnection) const override;
    };

    class SocketContextUpgradeInterface : public web::http::server::SocketContextUpgradeInterface {
    public:
        web::http::server::SocketContextUpgradeFactory* create() override;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H
