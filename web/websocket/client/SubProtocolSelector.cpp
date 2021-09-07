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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    std::shared_ptr<SubProtocolFactorySelector> SubProtocolFactorySelector::subProtocolSelector = nullptr;

    std::shared_ptr<SubProtocolFactorySelector> SubProtocolFactorySelector::instance() {
        if (subProtocolSelector == nullptr) {
            subProtocolSelector = std::make_shared<SubProtocolFactorySelector>();
        }

        return subProtocolSelector;
    }

    /*
        web::websocket::SubProtocol* SubProtocolSelector::select(const std::string& subProtocolName) {
            web::websocket::client::SubProtocol* subProtocol = nullptr;

            SubProtocolInterface* subProtocolInterface = dynamic_cast<SubProtocolInterface*>(selectSubProtocolFactory(subProtocolName));

            if (subProtocolInterface != nullptr) {
                subProtocol = dynamic_cast<SubProtocol*>(subProtocolInterface->create());
            }

            return subProtocol;
        }
        */

} // namespace web::websocket::client
