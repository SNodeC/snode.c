/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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
#include "express/middleware/VHost.h"
#include "express/tls/WebApp.h"
#include "log/Logger.h"

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

    legacy::WebApp6 legacyApp;

    legacyApp.use(middleware::VHost("localhost:8080").use(middleware::StaticMiddleware(SERVERROOT)));

    Router vHost = middleware::VHost("titan.home.vchrist.at:8080");
    vHost.get("/", [] APPLICATION(req, res) {
        res.send("Hello - I am VHOST titan.home.vchrist.at");
    });
    legacyApp.use(vHost);

    legacyApp.use([] APPLICATION(req, res) {
        res.sendStatus(404);
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    legacyApp.onConnect([](legacy::WebApp6::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnConnect:";

        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
    });

    legacyApp.onDisconnect([](legacy::WebApp6::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";

        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
    });

    tls::WebApp6 tlsApp({{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}});

    tlsApp.use(middleware::VHost("localhost:8088").use(getRouter()));

    vHost = middleware::VHost("titan.home.vchrist.at:8088");
    vHost.get("/", [] APPLICATION(req, res) {
        res.send("Hello - I am VHOST titan.home.vchrist.at");
    });
    tlsApp.use(vHost);

    tlsApp.use([] APPLICATION(req, res) {
        res.sendStatus(404);
    });

    tlsApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    tlsApp.onConnect([](tls::WebApp6::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnConnect:";

        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
    });

    tlsApp.onDisconnect([](tls::WebApp6::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";

        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
    });

    WebApp::start();
}
