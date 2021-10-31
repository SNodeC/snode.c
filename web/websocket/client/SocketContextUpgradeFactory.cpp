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

#include "web/websocket/client/SocketContextUpgradeFactory.h"

#include "utils/base64.h"
#include "web/http/client/Request.h"  // for Request
#include "web/http/client/Response.h" // for Response

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    void SocketContextUpgradeFactory::prepare(http::client::Request& request) {
        unsigned char ebytes[16];
        getentropy(ebytes, 16);

        request.set("Sec-WebSocket-Key", base64::base64_encode(ebytes, 16));
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    SocketContextUpgrade* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection,
                                                              [[maybe_unused]] web::http::client::Request* request,
                                                              web::http::client::Response* response) {
        std::string subProtocolName = response->header("sec-websocket-protocol");

        SocketContextUpgrade* socketContext = SocketContextUpgrade::create(this, socketConnection, subProtocolName);

        return socketContext;
    }

    void SocketContextUpgradeFactory::link() {
        static bool linked = false;

        if (!linked) {
            web::http::client::SocketContextUpgradeFactory::link("websocket", websocketClientContextUpgradeFactory);
            linked = true;
        }
    }

    extern "C" web::http::client::SocketContextUpgradeFactory* websocketClientContextUpgradeFactory() {
        return new SocketContextUpgradeFactory();
    }

} // namespace web::websocket::client
