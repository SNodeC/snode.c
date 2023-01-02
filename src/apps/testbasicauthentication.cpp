/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "express/legacy/in6/WebApp.h"
#include "express/middleware/BasicAuthentication.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "express/tls/in6/WebApp.h"
#include "log/Logger.h"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router getRouter(const std::string& webRoot) {
    Router router;

    router.use(middleware::BasicAuthentication("voc", "pentium5", "Authenticate yourself with username and password"));
    router.use(middleware::StaticMiddleware(webRoot));

    return router;
}

int main(int argc, char* argv[]) {
    std::string webRoot;
    utils::Config::add_option("--web-root", webRoot, "Root directory of the web site", true, "[path]");

    WebApp::init(argc, argv);

    {
        legacy::in6::WebApp legacyApp("legacy");

        Router& router = middleware::VHost("localhost:8080");

        Router& ba = middleware::BasicAuthentication("voc", "pentium5", "Authenticate yourself with username and password");
        ba.use(middleware::StaticMiddleware(webRoot));

        router.use(ba);
        legacyApp.use(router);

        router = middleware::VHost("atlas.home.vchrist.at:8080");
        router.get("/", [] APPLICATION(req, res) {
            res.send("Hello! I am VHOST atlas.home.vchrist.at.");
        });
        legacyApp.use(router);

        legacyApp.use([] APPLICATION(req, res) {
            res.status(404).send("The requested resource is not found.");
        });

        legacyApp.listen(8080, [](const legacy::in6::WebApp::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum < 0) {
                PLOG(ERROR) << "OnError";
            } else if (errnum > 0) {
                PLOG(ERROR) << "OnError: " << socketAddress.toString();
            } else {
                VLOG(0) << "snode.c listening on " << socketAddress.toString();
            }
        });

        {
            express::tls::in6::WebApp tlsApp("tls");

            Router& vh = middleware::VHost("localhost:8088");
            vh.use(getRouter(webRoot));
            tlsApp.use(vh);

            vh = middleware::VHost("atlas.home.vchrist.at:8088");
            vh.get("/", [] APPLICATION(req, res) {
                res.send("Hello! I am VHOST atlas.home.vchrist.at.");
            });

            tlsApp.use(vh);

            tlsApp.use([] APPLICATION(req, res) {
                res.status(404).send("The requested resource is not found.");
            });

            tlsApp.listen(8088, [](const legacy::in6::WebApp::SocketAddress& socketAddress, int errnum) -> void {
                if (errnum < 0) {
                    PLOG(ERROR) << "OnError";
                } else if (errnum > 0) {
                    PLOG(ERROR) << "OnError: " << socketAddress.toString();
                } else {
                    VLOG(0) << "snode.c listening on " << socketAddress.toString();
                }
            });
        }
    }

    WebApp::start();
}
