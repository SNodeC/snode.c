/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "web/websocket/server/SocketContextUpgrade.h"

#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgrade::SocketContextUpgrade(
        core::socket::stream::SocketConnection* socketConnection,
        web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory)
        : web::websocket::SocketContextUpgrade<SubProtocol, web::http::server::Request, web::http::server::Response>(
              socketConnection, socketContextUpgradeFactory, Role::SERVER) {
    }

    SocketContextUpgrade::~SocketContextUpgrade() {
        if (subProtocolFactory != nullptr && subProtocolFactory->deleteSubProtocol(subProtocol) == 0) {
            SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
        }
    }

    std::string SocketContextUpgrade::loadSubProtocol(const std::list<std::string>& subProtocolNames) {
        std::string selectedSubProtocolName;

        for (const std::string& subProtocolName : subProtocolNames) {
            subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::SERVER);

            if (subProtocolFactory != nullptr) {
                subProtocol = subProtocolFactory->createSubProtocol(this);
                selectedSubProtocolName = subProtocol != nullptr ? subProtocolName : "";
            }

            if (!selectedSubProtocolName.empty()) {
                break;
            }
        }

        return selectedSubProtocolName;
    }

} // namespace web::websocket::server
