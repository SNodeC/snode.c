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
#include "express/middleware/VHost.h"
#include "express/tls/WebApp.h"
#include "log/Logger.h"
#include "net/socket/ip/address/ipv6/InetAddress.h" // for InetAddress

#include <string> // for string, allocator

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

    {
        legacy::WebApp6 legacyApp;

        Router& router = middleware::VHost("localhost:8080");
        router.use(middleware::StaticMiddleware(SERVERROOT));
        legacyApp.use(router);

        router = middleware::VHost("titan.home.vchrist.at:8080");
        router.get("/", [] APPLICATION(req, res) {
            res.send("Hello! I am VHOST titan.home.vchrist.at.");
        });
        legacyApp.use(router);

        legacyApp.use([] APPLICATION(req, res) {
            res.status(404).send("The requested resource is not found.");
        });

        legacyApp.listen(8080, [](int err) -> void {
            if (err != 0) {
                PLOG(FATAL) << "listen on port 8080";
            } else {
                VLOG(0) << "snode.c listening on port 8080 for legacy connections";
            }
        });

        {
            tls::WebApp6 tlsApp({{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}});

            tlsApp.use(middleware::VHost("localhost:8088").use(getRouter()));

            tlsApp.use(middleware::VHost("titan.home.vchrist.at:8088").get("/", [] APPLICATION(req, res) {
                res.send("Hello! I am VHOST titan.home.vchrist.at.");
            }));

            tlsApp.use([] APPLICATION(req, res) {
                res.status(404).send("The requested resource is not found.");
            });

            tlsApp.onConnect([](const tls::WebApp6::SocketAddress& localAddress, const tls::WebApp6::SocketAddress& remoteAddress) -> void {
                VLOG(0) << "OnConnect:";

                VLOG(0) << "\tServer: " + localAddress.toString();
                VLOG(0) << "\tClient: " + remoteAddress.toString();
            });

            tlsApp.onDisconnect([](tls::WebApp6::SocketConnection* socketConnection) -> void {
                VLOG(0) << "OnDisconnect:";

                VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
                VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
            });

            tlsApp.listen(8088, [](int err) -> void {
                if (err != 0) {
                    PLOG(FATAL) << "listen on port 8088";
                } else {
                    VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
                }
            });
        }
    }

    WebApp::start();
}
