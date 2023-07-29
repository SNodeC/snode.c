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

#include "web/websocket/SubProtocolFactory.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocolFactorySelector* SubProtocolFactorySelector::instance() {
        static SubProtocolFactorySelector subProtocolFactorySelector;

        return &subProtocolFactorySelector;
    }

    SubProtocolFactorySelector::~SubProtocolFactorySelector() {
    }

    void SubProtocolFactorySelector::allowDlOpen() {
        SubProtocolFactorySelector::instance()->Super::allowDlOpen();
    }

    SubProtocolFactorySelector::SubProtocolFactory* SubProtocolFactorySelector::load(const std::string& subProtocolName) {
        std::string subProtocolLibraryFile =
            WEBSOCKET_SUBPROTOCO_INSTALL_LIBDIR "/libsnodec-websocket-" + subProtocolName + "-server.so." SOVERSION;
        std::string subProtocolFactoryFunctionName = subProtocolName + "ServerSubProtocolFactory";

        return Super::load(subProtocolName, subProtocolLibraryFile, subProtocolFactoryFunctionName);
    }

} // namespace web::websocket::server
