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

#include "utils/base64.h"
#include "web/http/server/Request.h"  // for Request
#include "web/http/server/Response.h" // for Response

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    SocketContextUpgrade* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection,
                                                       web::http::server::Request* request,
                                                       web::http::server::Response* response) {
        std::string subProtocolName = request->header("sec-websocket-protocol");

        SocketContextUpgrade* socketContext = SocketContextUpgrade::create(this, socketConnection, subProtocolName);

        if (socketContext != nullptr) {
            response->set("Upgrade", "websocket");
            response->set("Connection", "Upgrade");
            response->set("Sec-WebSocket-Protocol", subProtocolName);
            response->set("Sec-WebSocket-Accept", base64::serverWebSocketKey(request->header("sec-websocket-key")));

            response->status(101).end(); // Switch Protocol
        } else {
            response->set("Connection", "close");
            response->status(404).end(); // Not Found
        }

        return socketContext;
    }

    void SocketContextUpgradeFactory::link() {
        static bool linked = false;

        if (!linked) {
            web::http::server::SocketContextUpgradeFactory::link("websocket", websocketServerContextUpgradeFactory);
            linked = true;
        }
    }

    extern "C" web::http::server::SocketContextUpgradeFactory* websocketServerContextUpgradeFactory() {
        return new SocketContextUpgradeFactory();
    }

} // namespace web::websocket::server
