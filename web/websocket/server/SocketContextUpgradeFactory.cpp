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

#include "web/websocket/server/SocketContextUpgradeFactory.h"

#include "web/http/server/Request.h"  // for Request
#include "web/http/server/Response.h" // for Response
#include "web/http/server/SocketContextUpgradeFactorySelector.h"
#include "web/websocket/server/SubProtocol.h"
#include "web/websocket/server/SubProtocolFactorySelector.h" // for SubProt...
#include "web/websocket/ws_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory> // for __share...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SocketContextUpgradeFactory::~SocketContextUpgradeFactory() {
        SubProtocolFactorySelector::instance()->unload();
    }

    void SocketContextUpgradeFactory::attach(SubProtocolFactory* subProtocolFactory) {
        if (!web::http::server::SocketContextUpgradeFactorySelector::instance()->contains("websocket")) {
            web::http::server::SocketContextUpgradeFactorySelector::instance()->add(new SocketContextUpgradeFactory());
        }
        SubProtocolFactorySelector::instance()->add(subProtocolFactory);
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::server::SocketContextUpgradeFactory::Role SocketContextUpgradeFactory::role() {
        return http::server::SocketContextUpgradeFactory::Role::SERVER;
    }

    SocketContext* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection) const {
        std::string subProtocolName = request->header("sec-websocket-protocol");

        SocketContext* context = nullptr;

        web::websocket::server::SubProtocolFactory* subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocolName);

        if (subProtocolFactory != nullptr) {
            SubProtocol* subProtocol = subProtocolFactory->create();

            if (subProtocol != nullptr) {
                context = new SocketContext(socketConnection, subProtocol);
                subProtocol->setSocketContext(context);

                if (context != nullptr) {
                    response->set("Upgrade", "websocket");
                    response->set("Connection", "Upgrade");
                    response->set("Sec-WebSocket-Protocol", subProtocolName);

                    web::websocket::serverWebSocketKey(request->header("sec-websocket-key"),
                                                       [&response = this->response](char* key) -> void {
                                                           response->set("Sec-WebSocket-Accept", key);
                                                       });

                    response->status(101).end(); // Switch Protocol
                } else {
                    subProtocolFactory->destroy(subProtocol);
                    response->status(500).end(); // Internal Server Error
                }
            } else {
                response->status(404).end(); // Not Found
            }
        } else {
            response->status(404).end(); // Not Found
        }

        return context;
    }

    extern "C" {
        SocketContextUpgradeFactory* plugin() {
            return new SocketContextUpgradeFactory();
        }
    }

} // namespace web::websocket::server
