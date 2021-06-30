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

#include "SubProtocolSelector.h"

#include "SubProtocol.h"
#include "config.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <map>     // for map, _Rb_tree_iterator
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    std::shared_ptr<SubProtocolSelector> SubProtocolSelector::subProtocolSelector = nullptr;

    std::shared_ptr<SubProtocolSelector> SubProtocolSelector::instance() {
        if (subProtocolSelector == nullptr) {
            subProtocolSelector = std::make_shared<SubProtocolSelector>();
        }

        return subProtocolSelector;
    }

    SubProtocolSelector::SubProtocolSelector()
        : web::websocket::SubProtocolSelector(SubProtocolInterface::Role::SERVER) {
    }

    void SubProtocolSelector::loadSubProtocols() {
#ifndef NDEBUG
#ifdef SUBPROTOCOL_CLIENT_PATH
        loadSubProtocols(SUBPROTOCOL_CLIENT_PATH);
#endif // SUBPROTOCOL_CLIENT_PATH
#endif // NDEBUG

#ifdef SUBPROTOCOL_SERVER_INSTALL_PATH
        loadSubProtocols(SUBPROTOCOL_CLIENT_INSTALL_PATH);
#endif
        loadSubProtocols("/usr/local/lib/snode.c/" RELATIVE_SUBPROTOCOL_CLIENT_PATH);
        loadSubProtocols("/usr/lib/snode.c/" RELATIVE_SUBPROTOCOL_CLIENT_PATH);
    }

    web::websocket::SubProtocol* SubProtocolSelector::select(const std::string& subProtocolName) {
        web::websocket::client::SubProtocol* subProtocol = nullptr;

        SubProtocolInterface* subProtocolInterface = dynamic_cast<SubProtocolInterface*>(selectSubProtocolInterface(subProtocolName));

        if (subProtocolInterface != nullptr) {
            subProtocol = dynamic_cast<SubProtocol*>(subProtocolInterface->create());
        }

        return subProtocol;
    }

} // namespace web::websocket::client
