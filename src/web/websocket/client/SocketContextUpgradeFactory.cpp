/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "web/websocket/client/SocketContextUpgradeFactory.h"

#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/websocket/client/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/base64.h"

#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    void SocketContextUpgradeFactory::prepare(http::client::Request& request) {
        unsigned char ebytes[16];
        getentropy(ebytes, 16);

        request.set("Sec-WebSocket-Key", base64::base64_encode(ebytes, 16));
        request.set("Sec-WebSocket-Version", "13");
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::SocketContextUpgrade<web::http::client::Request, web::http::client::Response>*
    SocketContextUpgradeFactory::create(core::socket::stream::SocketConnection* socketConnection,
                                        web::http::client::Request* request,
                                        web::http::client::Response* response) {
        SocketContextUpgrade* socketContext = nullptr;

        if (response->get("sec-websocket-accept") == base64::serverWebSocketKey(request->header("Sec-WebSocket-Key"))) {
            const std::string subProtocolName = response->get("sec-websocket-protocol");

            socketContext = new SocketContextUpgrade(socketConnection, this);
            const std::string selectedSubProtocolName = socketContext->loadSubProtocol(subProtocolName);

            if (selectedSubProtocolName.empty()) {
                delete socketContext;
                socketContext = nullptr;
            }
        } else {
            checkRefCount();
        }

        return socketContext;
    }

    void SocketContextUpgradeFactory::link() {
        static bool linked = false;

        if (!linked) {
            web::http::client::SocketContextUpgradeFactory::link("websocket", websocketClientSocketContextUpgradeFactory);
            linked = true;
        }
    }

    extern "C" web::http::client::SocketContextUpgradeFactory* websocketClientSocketContextUpgradeFactory() {
        return new SocketContextUpgradeFactory();
    }

} // namespace web::websocket::client
