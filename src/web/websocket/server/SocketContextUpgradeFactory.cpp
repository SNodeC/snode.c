/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/websocket/server/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/base64.h"
#include "web/http/http_utils.h"

#include <list>
#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::SocketContextUpgrade<web::http::server::Request, web::http::server::Response>*
    SocketContextUpgradeFactory::create(core::socket::stream::SocketConnection* socketConnection,
                                        web::http::server::Request* request,
                                        web::http::server::Response* response) {
        SocketContextUpgrade* socketContext = nullptr;

        if (request->get("Sec-WebSocket-Version") == "13") {
            std::string requestedSubProtocolNames = request->get("sec-websocket-protocol");

            std::list<std::string> subProtocolNamesList;
            do {
                std::string subProtocolName;
                std::tie(subProtocolName, requestedSubProtocolNames) = httputils::str_split(requestedSubProtocolNames, ',');
                httputils::str_trimm(subProtocolName);
                subProtocolNamesList.push_back(subProtocolName);
            } while (!requestedSubProtocolNames.empty());

            if (!subProtocolNamesList.empty()) {
                std::string selectedSubProtocolName;

                socketContext = new SocketContextUpgrade(socketConnection, this);
                selectedSubProtocolName = socketContext->loadSubProtocol(subProtocolNamesList);

                if (!selectedSubProtocolName.empty()) {
                    response->set("Upgrade", "websocket");
                    response->set("Connection", "Upgrade");
                    response->set("Sec-WebSocket-Protocol", selectedSubProtocolName);
                    response->set("Sec-WebSocket-Accept", base64::serverWebSocketKey(request->get("sec-websocket-key")));

                    response->status(101); // Switch Protocol
                } else {
                    delete socketContext;
                    socketContext = nullptr;

                    response->set("Connection", "close");
                    response->status(400);
                }
            } else {
                checkRefCount();

                response->set("Connection", "close");
                response->status(400);
            }
        } else {
            checkRefCount();

            response->set("Sec-WebSocket-Version", "13");
            response->set("Connection", "close");
            response->status(426);
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
