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

#include "WSContext.h"

#include "log/Logger.h"
#include "web/ws/WSSubProtocol.h" // for WSSubProtocol, WSSubProtocol::Role, WSProto...
#include "web/ws/server/WSSubProtocolPluginInterface.h"
#include "web/ws/server/WSSubProtocolSelector.h"
#include "web/ws/ws_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws::server {

    WSContext::WSContext(web::ws::server::WSSubProtocol* wSSubProtocol, web::ws::WSSubProtocol::Role role)
        : web::ws::WSContext(wSSubProtocol, role) {
    }

    WSContext::WSContext::~WSContext() {
        web::ws::WSSubProtocolPluginInterface* wSSubProtocolPluginInterface =
            web::ws::server::WSSubProtocolSelector::instance().select(wSSubProtocol->getName());

        if (wSSubProtocolPluginInterface != nullptr) {
            wSSubProtocolPluginInterface->destroy(dynamic_cast<web::ws::server::WSSubProtocol*>(wSSubProtocol));
        }
    }

    WSContext* WSContext::create(web::http::server::Request& req, web::http::server::Response& res) {
        std::string subProtocol = req.header("sec-websocket-protocol");

        web::ws::server::WSSubProtocolPluginInterface* wSSubProtocolPluginInterface =
            web::ws::server::WSSubProtocolSelector::instance().select(subProtocol);

        web::ws::server::WSContext* wSContext = nullptr;

        if (wSSubProtocolPluginInterface != nullptr) {
            web::ws::server::WSSubProtocol* wSSubProtocol =
                static_cast<web::ws::server::WSSubProtocol*>(wSSubProtocolPluginInterface->create(req, res));

            if (wSSubProtocol != nullptr) {
                wSSubProtocol->setClients(wSSubProtocolPluginInterface->getClients());

                wSContext = new web::ws::server::WSContext(wSSubProtocol, web::ws::WSSubProtocol::Role::SERVER);

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
