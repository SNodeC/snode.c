/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/websocket/server/SocketContextUpgradeFactory.h"

#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/server/SocketContextUpgrade.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

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

    SubProtocol* SocketContextUpgradeFactory::loadSubProtocol(const std::list<std::string>& subProtocolNames, int val) {
        SubProtocol* subProtocol = nullptr;

        for (const std::string& subProtocolName : subProtocolNames) {
            web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory =
                SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::CLIENT);

            if (subProtocolFactory != nullptr) {
                VLOG(0) << "--------------- Loaded subprotocol 1 " << subProtocolName;
                subProtocol = subProtocolFactory->createSubProtocol(val);
                VLOG(0) << "--------------- Loaded subprotocol 2 " << subProtocolName;

                if (subProtocol != nullptr) {
                    break;
                }
            }
        }

        return subProtocol;
    }

    http::SocketContextUpgrade<web::http::server::Request, web::http::server::Response>*
    SocketContextUpgradeFactory::create(core::socket::stream::SocketConnection* socketConnection,
                                        web::http::server::Request* request,
                                        web::http::server::Response* response,
                                        int val) {
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
                SubProtocol* subProtocol = loadSubProtocol(subProtocolNamesList, val);

                if (subProtocol != nullptr) {
                    socketContext = new SocketContextUpgrade(socketConnection, subProtocol, this);

                    response->set("Upgrade", "websocket");
                    response->set("Connection", "Upgrade");
                    response->set("Sec-WebSocket-Protocol", subProtocol->getName());
                    response->set("Sec-WebSocket-Accept", base64::serverWebSocketKey(request->get("sec-websocket-key")));

                    response->status(101); // Switch Protocol
                } else {
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
            web::http::server::SocketContextUpgradeFactory::link("websocket", websocketServerSocketContextUpgradeFactory);
            linked = true;
        }
    }

    extern "C" web::http::server::SocketContextUpgradeFactory* websocketServerSocketContextUpgradeFactory() {
        return new SocketContextUpgradeFactory();
    }

} // namespace web::websocket::server
