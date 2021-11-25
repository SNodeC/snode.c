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

#include "web/websocket/server/SocketContextUpgrade.h"

#include "web/websocket/server/SubProtocolFactory.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgrade::SocketContextUpgrade(
        core::socket::stream::SocketConnection* socketConnection,
        web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
        SubProtocol* subProtocol)
        : web::websocket::SocketContextUpgrade<SubProtocol, web::http::server::Request, web::http::server::Response>(
              socketConnection, socketContextUpgradeFactory, subProtocol, core::socket::stream::SocketContext::Role::SERVER) {
        subProtocol->setSocketContextUpgrade(this);
    }

    SocketContextUpgrade* SocketContextUpgrade::create(
        web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory,
        core::socket::stream::SocketConnection* socketConnection,
        const std::string& subProtocolName) {
        SocketContextUpgrade* socketContextUpgrade = nullptr;

        SubProtocolFactory* subProtocolFactory =
            SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::SERVER);

        if (subProtocolFactory != nullptr) {
            SubProtocol* subProtocol = subProtocolFactory->createSubProtocol();

            if (subProtocol != nullptr) {
                socketContextUpgrade = new SocketContextUpgrade(socketConnection, socketContextUpgradeFactory, subProtocol);

                if (socketContextUpgrade == nullptr && subProtocolFactory->deleteSubProtocol(subProtocol) == 0) {
                    SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
                }
            }
        }

        return socketContextUpgrade;
    }

    SocketContextUpgrade::~SocketContextUpgrade() {
        SubProtocolFactory* subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocol->getName());

        if (subProtocolFactory != nullptr && subProtocolFactory->deleteSubProtocol(subProtocol) == 0) {
            SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
        }
    }

} // namespace web::websocket::server
