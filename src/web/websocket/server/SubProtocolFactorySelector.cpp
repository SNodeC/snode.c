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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {
    SubProtocolFactorySelector::SubProtocolFactorySelector() {
        web::websocket::SubProtocolFactorySelector<SubProtocolFactory>::addSubProtocolSearchPath(
            WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR);

#if !defined(NDEBUG) && defined(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR)

        web::websocket::SubProtocolFactorySelector<SubProtocolFactory>::addSubProtocolSearchPath(
            WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR);

#endif // !defined(NDEBUG) && defined(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR)
    }

    SubProtocolFactorySelector* SubProtocolFactorySelector::instance() {
        static SubProtocolFactorySelector subProtocolFactorySelector;

        return &subProtocolFactorySelector;
    }

    void SubProtocolFactorySelector::link(const std::string& subProtocolName, void* (*getSubProtocolFactory)()) {
        SubProtocolFactorySelector::instance()->linkSubProtocolFactory(subProtocolName, getSubProtocolFactory);
    }

    void SubProtocolFactorySelector::addSubProtocolSearchPath(const std::string& searchPath) {
        SubProtocolFactorySelector::instance()->web::websocket::SubProtocolFactorySelector<SubProtocolFactory>::addSubProtocolSearchPath(
            searchPath);
    }

    void SubProtocolFactorySelector::allowDlOpen() {
        SubProtocolFactorySelector::instance()->web::websocket::SubProtocolFactorySelector<SubProtocolFactory>::allowDlOpen();
    }

} // namespace web::websocket::server
