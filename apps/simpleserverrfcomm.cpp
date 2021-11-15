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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "config.h" // just for this example app
#include "express/legacy/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/tls/WebApp.h"
#include "log/Logger.h"
#include "net/socket/bluetooth/address/RfCommAddress.h" // for RfCommAddress

#include <string> // for string, operator+

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router getRouter() {
    Router router;

    router.use(middleware::StaticMiddleware(SERVERROOT));

    return router;
}

int main(int argc, char* argv[]) {
    Logger::setVerboseLevel(2);

    WebApp::init(argc, argv);

    legacy::WebAppRfComm legacyApp(getRouter());

    legacyApp.onConnect(
        [](const legacy::WebAppRfComm::SocketAddress& localAddress, const legacy::WebAppRfComm::SocketAddress& remoteAddress) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: (" + localAddress.address() + ") " + localAddress.toString();
            VLOG(0) << "\tClient: (" + remoteAddress.address() + ") " + remoteAddress.toString();
        });

    legacyApp.onDisconnect([](legacy::WebAppRfComm::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";

        VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " + socketConnection->getRemoteAddress().toString();
    });

    legacyApp.listen("A4:B1:C1:2C:82:37", 1, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "BT listen: " << err;
        } else {
            LOG(INFO) << "BT (legacy) listening on channel 1";
        }
    });

    tls::WebAppRfComm tlsApp(getRouter(), {{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}});

    tlsApp.onConnect(
        [](const tls::WebAppRfComm::SocketAddress& localAddress, const tls::WebAppRfComm::SocketAddress& remoteAddress) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: (" + localAddress.address() + ") " + localAddress.toString();
            VLOG(0) << "\tClient: (" + remoteAddress.address() + ") " + remoteAddress.toString();
        });

    tlsApp.onDisconnect([](tls::WebAppRfComm::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";

        VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " + socketConnection->getRemoteAddress().toString();
    });

    tlsApp.addSniCert("myhost", {{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}, {"caFile", CLIENTCAFILE}});

    tlsApp.listen("A4:B1:C1:2C:82:37", 2, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "BT listen: " << err;
        } else {
            LOG(INFO) << "BT (tls) listening on channel 2";
        }
    });

    return WebApp::start();
}
