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

#include "SocketContextUpgradeFactory.h"

#include "SubProtocol.h"
#include "utils/base64.h"
#include "web/http/server/Request.h"  // for Request
#include "web/http/server/Response.h" // for Response
#include "web/http/server/SocketContextUpgradeFactorySelector.h"
#include "web/websocket/server/SocketContext.h" // for Soc...
#include "web/websocket/server/SubProtocolFactory.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    void SocketContextUpgradeFactory::deleted(SocketContext* socketContext) {
        SubProtocolFactory* subProtocolFactory = socketContext->getSubProtocol()->getSubProtocolFactory();

        if (subProtocolFactory->deleteSubProtocol(socketContext->getSubProtocol()) == 0) {
            SubProtocolFactorySelector::instance()->unload(subProtocolFactory);
        }

        --refCount;
        if (refCount == 0) {
            web::http::server::SocketContextUpgradeFactorySelector::instance()->unload(this);
        }
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    SocketContext* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection) {
        std::string subProtocolName = request->header("sec-websocket-protocol");

        SocketContext* socketContext = nullptr;

        web::websocket::server::SubProtocolFactory* subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocolName);

        if (subProtocolFactory != nullptr) {
            SubProtocol* subProtocol = subProtocolFactory->createSubProtocol();

            if (subProtocol != nullptr) {
                socketContext = new SocketContext(socketConnection, subProtocol);

                if (socketContext != nullptr) {
                    refCount++;

                    socketContext->setSocketContextUpgradeFactory(this);
                    subProtocol->setSocketContext(socketContext);
                    subProtocol->setSubProtocolFactory(subProtocolFactory);

                    response->set("Upgrade", "websocket");
                    response->set("Connection", "Upgrade");
                    response->set("Sec-WebSocket-Protocol", subProtocolName);
                    response->set("Sec-WebSocket-Accept", base64::serverWebSocketKey(request->header("sec-websocket-key")));

                    response->status(101).end(); // Switch Protocol
                } else {
                    subProtocolFactory->deleteSubProtocol(subProtocol);
                    response->set("Connection", "close");
                    response->status(500).end(); // Internal Server Error
                }
            } else {
                response->set("Connection", "close");
                response->status(404).end(); // Not Found
            }
        } else {
            response->set("Connection", "close");
            response->status(404).end(); // Not Found
        }

        if (refCount == 0) {
            web::http::server::SocketContextUpgradeFactorySelector::instance()->unload(this);
        }

        return socketContext;
    }

    extern "C" {
        web::http::server::SocketContextUpgradeFactory* getSocketContextUpgradeFactory() {
            return new SocketContextUpgradeFactory();
        }

        void linkStatic(const std::string& subProtocolName, web::websocket::server::SubProtocolFactory* (*getSubProtocolFactory)()) {
            web::websocket::server::SubProtocolFactorySelector::linkStatic(subProtocolName, getSubProtocolFactory);
            web::http::server::SocketContextUpgradeFactorySelector::instance()->setLinkedPlugin("websocket",
                                                                                                getSocketContextUpgradeFactory);
        }
    }

} // namespace web::websocket::server
