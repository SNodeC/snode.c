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

#include "web/websocket/server/SubProtocolFactorySelector.h"

#include "config.h"
#include "web/websocket/server/SocketContextUpgradeFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocolFactorySelector::SubProtocolFactorySelector() {
#ifndef NDEBUG
#ifdef SUBPROTOCOL_SERVER_COMPILE_PATH

        addSubProtocolSearchPath(SUBPROTOCOL_SERVER_COMPILE_PATH);

#endif // SUBPROTOCOL_SERVER_COMPILE_PATH
#endif // NDEBUG

        addSubProtocolSearchPath(SUBPROTOCOL_SERVER_INSTALL_PATH);
    }

    SubProtocolFactorySelector* SubProtocolFactorySelector::instance() {
        static SubProtocolFactorySelector subProtocolFactorySelector;

        return &subProtocolFactorySelector;
    }

    void linkSubProtocol(const std::string& subProtocolName, web::websocket::server::SubProtocolFactory* (*getSubProtocolFactory)()) {
        linkWebsocketServer();
        web::websocket::server::SubProtocolFactorySelector::instance()->linkSubProtocol(subProtocolName, getSubProtocolFactory);
    }

} // namespace web::websocket::server
