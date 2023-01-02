/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocolFactorySelector::SubProtocolFactorySelector() {
        Super::addSubProtocolSearchPath(WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR);

#if !defined(NDEBUG) && defined(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR)

        Super::addSubProtocolSearchPath(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR);

#endif // !defined(NDEBUG) && defined(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR)

        char* envDlWsServerSubprotocolPath = getenv("DL_WSSERVER_SUBPROTOCOL_PATH");
        if (envDlWsServerSubprotocolPath != nullptr) {
            Super::addSubProtocolSearchPath(envDlWsServerSubprotocolPath);
        }
    }

    SubProtocolFactorySelector* SubProtocolFactorySelector::instance() {
        static SubProtocolFactorySelector subProtocolFactorySelector;

        return &subProtocolFactorySelector;
    }

    void SubProtocolFactorySelector::link(const std::string& subProtocolName, void* (*getSubProtocolFactory)()) {
        SubProtocolFactorySelector::instance()->Super::link(subProtocolName, getSubProtocolFactory);
    }

    void SubProtocolFactorySelector::addSubProtocolSearchPath(const std::string& searchPath) {
        SubProtocolFactorySelector::instance()->Super::addSubProtocolSearchPath(searchPath);
    }

    void SubProtocolFactorySelector::allowDlOpen() {
        SubProtocolFactorySelector::instance()->Super::allowDlOpen();
    }

} // namespace web::websocket::server
