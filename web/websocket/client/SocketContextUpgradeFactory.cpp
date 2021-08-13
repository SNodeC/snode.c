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

#include "SocketContextUpgradeFactory.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::client {

    extern "C" {
        class web::ws::client::SocketContextUpgradeInterface* plugin() {
            return new SocketContextUpgradeInterface();
        }
    }

    http::server::SocketContextUpgradeFactory* SocketContextUpgradeInterface::create() {
        return new SocketContextUpgradeFactory();
    }

    SocketContextUpgradeFactory::SocketContextUpgradeFactory() {
        web::ws::server::SubProtocolSelector::instance()->loadSubProtocols();
    }

    SocketContextUpgradeFactory::~SocketContextUpgradeFactory() {
        web::ws::server::SubProtocolSelector::instance()->unloadSubProtocols();
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::server::SocketContextUpgradeFactory::ROLE SocketContextUpgradeFactory::role() {
        return http::server::SocketContextUpgradeFactory::ROLE::SERVER;
    }

    web::ws::server::SocketContext* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnectionBase* socketConnection) const {
        return web::ws::server::SocketContext::create(socketConnection, *request, *response);
    }

} // namespace web::ws::client
