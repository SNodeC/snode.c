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

#include "web/websocket/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SubProtocolFactory.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgrade::SocketContextUpgrade(net::socket::stream::SocketConnection* socketConnection,
                                               SocketContextUpgradeFactory* socketContextUpgradeFactory,
                                               SubProtocol* subProtocol)
        : web::websocket::SocketContextUpgrade<web::http::server::Request, web::http::server::Response, SubProtocol>(
              socketConnection, socketContextUpgradeFactory, subProtocol, Role::SERVER) {
        subProtocol->setSocketContextUpgrade(this);
    }

    SocketContextUpgrade* SocketContextUpgrade::create(SocketContextUpgradeFactory* socketContextUpgradeFactory,
                                                       net::socket::stream::SocketConnection* socketConnection,
                                                       const std::string& subProtocolName) {
        SocketContextUpgrade* socketContext = nullptr;

        SubProtocolFactory* subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocolName);

        if (subProtocolFactory != nullptr) {
            SubProtocol* subProtocol = subProtocolFactory->createSubProtocol();

            if (subProtocol != nullptr) {
                subProtocol->setSubProtocolFactory(subProtocolFactory);

                socketContext = new SocketContextUpgrade(socketConnection, socketContextUpgradeFactory, subProtocol);

                if (socketContext == nullptr) {
                    subProtocolFactory->deleteSubProtocol(subProtocol);
                }
            }
        }

        return socketContext;
    }

    SocketContextUpgrade::~SocketContextUpgrade() {
        SocketContextUpgrade::SubProtocol* subProtocol = getSubProtocol();

        SocketContextUpgrade::SubProtocol::SubProtocolFactory* subProtocolFactory = subProtocol->getSubProtocolFactory();
        subProtocolFactory->deleteSubProtocol(subProtocol);
    }

} // namespace web::websocket::server
