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

#include "SubProtocolPluginInterface.h" // for WSSubPr...
#include "SubProtocolSelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map, _Rb_tree_iterator
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    SubProtocolSelector SubProtocolSelector::wSSubProtocolSelector;

    SubProtocolSelector& SubProtocolSelector::instance() {
        return web::ws::server::SubProtocolSelector::wSSubProtocolSelector;
    }

    web::ws::server::SubProtocolPluginInterface* SubProtocolSelector::select(const std::string& subProtocolName) {
        SubProtocolPluginInterface* wSSubProtocolPluginInterface = nullptr;

        if (serverSubprotocols.contains(subProtocolName)) {
            wSSubProtocolPluginInterface =
                static_cast<SubProtocolPluginInterface*>(serverSubprotocols.find(subProtocolName)->second.wSSubprotocolPluginInterface);
        }

        return wSSubProtocolPluginInterface;
    }

} // namespace web::ws::server
