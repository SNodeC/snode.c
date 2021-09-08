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

#include "web/websocket/server/SocketContextUpgradeFactory.h"

#include "web/websocket/server/SubProtocolFactory.h"         // for SubProtocolFactory
#include "web/websocket/server/SubProtocolFactorySelector.h" // for SubProt...

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory> // for __share...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgradeFactory::SocketContextUpgradeFactory(web::websocket::server::SubProtocolFactory* subProtocolFactory) {
        SubProtocolFactorySelector::instance()->add(subProtocolFactory);
    }

    SocketContextUpgradeFactory::~SocketContextUpgradeFactory() {
        SubProtocolFactorySelector::instance()->unload();
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::server::SocketContextUpgradeFactory::Role SocketContextUpgradeFactory::role() {
        return http::server::SocketContextUpgradeFactory::Role::SERVER;
    }

    SocketContext* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection) const {
        return SocketContext::create(socketConnection, *request, *response);
    }

    extern "C" {
        SocketContextUpgradeFactory* plugin() {
            return new SocketContextUpgradeFactory();
        }
    }

} // namespace web::websocket::server
