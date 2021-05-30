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

#ifndef WEB_WS_SERVER_WSSERVERCONTEXT_HPP
#define WEB_WS_SERVER_WSSERVERCONTEXT_HPP

#include "WSContext.h"

#include "log/Logger.h"
#include "web/ws/WSSubProtocol.h" // for WSProtocol, WSProtocol::Role, WSProto...
#include "web/ws/subprotocol/WSSubProtocolSelector.h"
#include "web/ws/ws_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws::server {

    WSContext::WSContext(web::ws::WSSubProtocol* wSProtocol, web::ws::WSSubProtocol::Role role)
        : web::ws::WSContext(wSProtocol, role) {
    }

    WSContext::WSContext::~WSContext() {
        web::ws::subprotocol::WSSubProtocolPluginInterface* wSProtocolPlugin =
            selector.select(wSProtocol->getName(), web::ws::WSSubProtocol::Role::SERVER);

        if (wSProtocolPlugin != nullptr) {
            wSProtocolPlugin->destroy(wSProtocol);
        }
    }

    WSContext* WSContext::create(web::http::server::Request& req, web::http::server::Response& res) {
        std::string subProtocol = req.header("sec-websocket-protocol");

        web::ws::subprotocol::WSSubProtocolPluginInterface* wSProtocolPlugin =
            selector.select(subProtocol, web::ws::WSSubProtocol::Role::SERVER);

        web::ws::server::WSContext* wSContext = nullptr;

        if (wSProtocolPlugin != nullptr) {
            web::ws::WSSubProtocol* wSProtocol = wSProtocolPlugin->create();

            if (wSProtocol != nullptr) {
                wSContext = new web::ws::server::WSContext(wSProtocol, web::ws::WSSubProtocol::Role::SERVER);

                if (wSContext != nullptr) {
                    res.set("Upgrade", "websocket");
                    res.set("Connection", "Upgrade");
                    res.set("Sec-WebSocket-Protocol", subProtocol);

                    web::ws::serverWebSocketKey(req.header("sec-websocket-key"), [&res](char* key) -> void {
                        res.set("Sec-WebSocket-Accept", key);
                    });

                    res.status(101); // Switch Protocol
                } else {
                    res.status(500);
                }
            } else {
                res.status(500); // Internal Server Error
            }
        } else {
            res.status(404); // Not found
        }

        return wSContext;
    }

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_HPP
