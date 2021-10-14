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
#include "web/http/client/Request.h"  // for Request
#include "web/http/client/Response.h" // for Response
#include "web/http/client/SocketContextUpgradeFactorySelector.h"
#include "web/websocket/SocketContext.h"                     // for Soc...
#include "web/websocket/client/SubProtocolFactorySelector.h" // for Sub...

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unistd.h>

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
            SubProtocolFactorySelector::instance()->add(subProtocolFactory);
        }
    }

    void SocketContextUpgradeFactory::prepare(http::client::Request& request, http::client::Response& response) {
        http::client::SocketContextUpgradeFactory::prepare(request, response);
        unsigned char ebytes[16];
        getentropy(ebytes, 16);

        request.set("Sec-WebSocket-Key", base64::base64_encode(ebytes, 16));
    }

    void SocketContextUpgradeFactory::deleted(SocketContext* socketContext) {
        SubProtocolFactory* subProtocolFactory =
            dynamic_cast<SubProtocolFactory*>(socketContext->getSubProtocol()->getSubProtocolFactory());

        if (subProtocolFactory->deleted(socketContext->getSubProtocol()) == 0) {
            SubProtocolFactorySelector::instance()->unload(dynamic_cast<web::websocket::client::SubProtocolFactory*>(subProtocolFactory));
        }

        --refCount;
        if (refCount == 0) {
            web::http::client::SocketContextUpgradeFactorySelector::instance()->unused(this);
        }
    }

    std::string SocketContextUpgradeFactory::name() {
        return "websocket";
    }

    http::client::SocketContextUpgradeFactory::Role SocketContextUpgradeFactory::role() {
        return http::client::SocketContextUpgradeFactory::Role::CLIENT;
    }

    SocketContext* SocketContextUpgradeFactory::create(net::socket::stream::SocketConnection* socketConnection) {
        std::string subProtocolName = response->header("sec-websocket-protocol");

        SocketContext* socketContext = nullptr;

        web::websocket::client::SubProtocolFactory* subProtocolFactory = SubProtocolFactorySelector::instance()->select(subProtocolName);

        if (subProtocolFactory != nullptr) {
            SubProtocol* subProtocol = subProtocolFactory->createSubProtocol();

            if (subProtocol != nullptr) {
                socketContext = new SocketContext(socketConnection, subProtocol);

                if (socketContext != nullptr) {
                    refCount++;

                    socketContext->setSocketContextUpgradeFactory(this);
                    subProtocol->setSocketContext(socketContext);
                    subProtocol->setSubProtocolFactory(subProtocolFactory);

                } else {
                    delete subProtocol;
                }
            }
        }

        if (refCount == 0) {
            web::http::client::SocketContextUpgradeFactorySelector::instance()->unused(this);
        }

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
