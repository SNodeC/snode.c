/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include "web/websocket/server/SubProtocol.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgrade::SocketContextUpgrade(
        core::socket::SocketConnection* socketConnection,
        web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
        web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory)
        : web::websocket::SocketContextUpgrade<SubProtocol, web::http::server::Request, web::http::server::Response>(
              socketConnection, socketContextUpgradeFactory, subProtocolFactory, Role::SERVER) {
    }

    SocketContextUpgrade* SocketContextUpgrade::create(
        web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
        core::socket::SocketConnection* socketConnection,
        const std::string& subProtocolName) {
        SocketContextUpgrade* socketContextUpgrade = nullptr;

        web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory =
            SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::SERVER);

        if (subProtocolFactory != nullptr) {
            socketContextUpgrade = new SocketContextUpgrade(socketConnection, socketContextUpgradeFactory, subProtocolFactory);

            if (socketContextUpgrade->subProtocol == nullptr) {
                delete socketContextUpgrade;
                socketContextUpgrade = nullptr;
            }
        }
        /*
                if (subProtocolFactory != nullptr) {
                    SubProtocol* subProtocol = subProtocolFactory->createSubProtocol();

                    if (subProtocol != nullptr) {
                        socketContextUpgrade = new SocketContextUpgrade(socketConnection, socketContextUpgradeFactory, subProtocol);

                        if (socketContextUpgrade == nullptr && subProtocolFactory->deleteSubProtocol(subProtocol) == 0) {
                            SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
                        }
                    }
                }
        */
        return socketContextUpgrade;
    }

    SocketContextUpgrade::~SocketContextUpgrade() {
        web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory =
            SubProtocolFactorySelector::instance()->select(subProtocol->getName());

        if (subProtocolFactory != nullptr && subProtocolFactory->deleteSubProtocol(subProtocol) == 0) {
            SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
        }
    }

} // namespace web::websocket::server
