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

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router getRouter(const std::string& webRoot) {
    Router router;

    router.use(middleware::BasicAuthentication("voc", "pentium5", "Authenticate yourself with username and password"));
    router.use(middleware::StaticMiddleware(webRoot));

    return router;
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--web-root", "Root directory of the web site", "[path]");

    WebApp::init(argc, argv);

    {
        legacy::in6::WebApp legacyApp("legacy");

        Router& router1 = middleware::VHost("localhost:8080");

        Router& ba = middleware::BasicAuthentication("voc", "pentium5", "Authenticate yourself with username and password");
        ba.use(middleware::StaticMiddleware(utils::Config::get_string_option_value("--web-root")));

        router1.use(ba);
        legacyApp.use(router1);

        Router& router2 = middleware::VHost("atlas.home.vchrist.at:8080");
        router2.get("/", [] APPLICATION(req, res) {
            res.send("Hello! I am VHOST atlas.home.vchrist.at.");
        });
        legacyApp.use(router2);

        legacyApp.use([] APPLICATION(req, res) {
            res.status(404).send("The requested resource is not found.");
        });

        legacyApp.listen(8080,
                         [instanceName = legacyApp.getConfig().getInstanceName()](const legacy::in6::WebApp::SocketAddress& socketAddress,
                                                                                  const core::socket::State& state) -> void {
                             switch (state) {
                                 case core::socket::State::OK:
                                     VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
                                     break;
                                 case core::socket::State::DISABLED:
                                     VLOG(1) << instanceName << ": disabled";
                                     break;
                                 case core::socket::State::ERROR:
                                     LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                     break;
                                 case core::socket::State::FATAL:
                                     LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                     break;
                             }
                         });

        {
            express::tls::in6::WebApp tlsApp("tls");

            Router& vh = middleware::VHost("localhost:8088");
            vh.use(getRouter(utils::Config::get_string_option_value("--web-root")));
            tlsApp.use(vh);

            vh = middleware::VHost("atlas.home.vchrist.at:8088");
            vh.get("/", [] APPLICATION(req, res) {
                res.send("Hello! I am VHOST atlas.home.vchrist.at.");
            });

            tlsApp.use(vh);

            tlsApp.use([] APPLICATION(req, res) {
                res.status(404).send("The requested resource is not found.");
            });

            tlsApp.listen(8088, [](const legacy::in6::WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << "tls: listening on '" << socketAddress.toString() << "'" << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << "tls: disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << "tls: error occurred";
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << "tls: fatal error occurred";
                        break;
                }
            });
        }
    }

    WebApp::start();
}
