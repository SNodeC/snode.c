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
#include "log/Logger.h"
#include "utils/base64.h"
#include "web/http/client/Request.h"  // for Request
#include "web/http/client/Response.h" // for Response
#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    void SocketContextUpgradeFactory::attach(SubProtocolFactory* subProtocolFactory) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = dynamic_cast<SocketContextUpgradeFactory*>(
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select("websocket", false));

        if (socketContextUpgradeFactory == nullptr) {
            socketContextUpgradeFactory = new SocketContextUpgradeFactory();
            web::http::client::SocketContextUpgradeFactorySelector::instance()->add(socketContextUpgradeFactory);
        }

        if (socketContextUpgradeFactory != nullptr) {
            socketContextUpgradeFactory->subProtocolFactorySelector.add(subProtocolFactory);
        }
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::client::SocketContextUpgradeFactory::Role SocketContextUpgradeFactory::role() {
        return http::client::SocketContextUpgradeFactory::Role::CLIENT;
    }

    SocketContext* SocketContextUpgradeFactory::create([[maybe_unused]] net::socket::stream::SocketConnection* socketConnection) {
        //        std::string subProtocolName = request->header("sec-websocket-protocol");

        std::string subProtocolName = "hihihihi";

        SocketContext* socketContext = nullptr;

        [[maybe_unused]] web::websocket::client::SubProtocolFactory* subProtocolFactory =
            subProtocolFactorySelector.select(subProtocolName);
        /*
                if (subProtocolFactory != nullptr) {
                    SubProtocol* subProtocol = subProtocolFactory->create();

                    if (subProtocol != nullptr) {
                        socketContext = new SocketContext(socketConnection, subProtocol);

                        if (socketContext != nullptr) {
                            response->set("Upgrade", "websocket");
                            response->set("Connection", "Upgrade");
                            response->set("Sec-WebSocket-Protocol", subProtocolName);
                            response->set("Sec-WebSocket-Accept", base64::serverWebSocketKey(request->header("sec-websocket-key")));

                            response->status(101).end(); // Switch Protocol
                        } else {
                            delete subProtocol;
                            response->status(500).end(); // Internal Server Error
                        }
                    } else {
                        response->status(404).end(); // Not Found
                    }
                } else {
                    response->status(404).end(); // Not Found
                }
        */
        return socketContext;
    }

    void SocketContextUpgradeFactory::destroy() {
        delete this;
    }

    extern "C" {
        SocketContextUpgradeFactory* plugin() {
            return new SocketContextUpgradeFactory();
        }
    }

} // namespace web::websocket::client
