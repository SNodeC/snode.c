/*
 * SNode.C - a slim toolkit for network communication
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

#include "express/legacy/in6/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "express/tls/in6/WebApp.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router getRouter(const std::string& webRoot) {
    Router router;

    router.use(middleware::StaticMiddleware(webRoot));

    return router;
}

int main(int argc, char* argv[]) {
    utils::Config::addStringOption("--web-root", "Root directory of the web site", "[path]");

    WebApp::init(argc, argv);

    {
        const legacy::in6::WebApp legacyApp("legacy");

        const Router& vh1 = middleware::VHost("localhost:8080");
        vh1.use(middleware::StaticMiddleware(utils::Config::getStringOptionValue("--web-root")));
        legacyApp.use(vh1);

        const Router& vh2 = middleware::VHost("atlas.home.vchrist.at:8080");
        vh2.get("/", [] APPLICATION(req, res) {
            res->send("Hello! I am VHOST atlas.home.vchrist.at.");
        });
        legacyApp.use(vh2);

        const Router& vh3 = middleware::VHost("ceres.home.vchrist.at:8080");
        vh3.get("/", [] APPLICATION(req, res) {
            res->send("Hello! I am VHOST ceres.home.vchrist.at.");
        });
        legacyApp.use(vh3);

        legacyApp.use([] APPLICATION(req, res) {
            res->status(404).send("The requested resource is not found.");
        });

        legacyApp.listen(8080,
                         [instanceName = legacyApp.getConfig().getInstanceName()](const legacy::in6::WebApp::SocketAddress& socketAddress,
                                                                                  const core::socket::State& state) {
                             switch (state) {
                                 case core::socket::State::OK:
                                     VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                                     break;
                                 case core::socket::State::DISABLED:
                                     VLOG(1) << instanceName << " disabled";
                                     break;
                                 case core::socket::State::ERROR:
                                     LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                     break;
                                 case core::socket::State::FATAL:
                                     LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                     break;
                             }
                         });
    }

    {
        const express::tls::in6::WebApp tlsApp("tls");

        const Router& vh1 = middleware::VHost("localhost:8088");
        vh1.use(getRouter(utils::Config::getStringOptionValue("--web-root")));
        tlsApp.use(vh1);

        const Router& vh2 = middleware::VHost("atlas.home.vchrist.at:8088");
        vh2.get("/", [] APPLICATION(req, res) {
            res->send("Hello! I am VHOST atlas.home.vchrist.at.");
        });
        tlsApp.use(vh2);

        const Router& vh3 = middleware::VHost("ceres.home.vchrist.at:8080");
        vh3.get("/", [] APPLICATION(req, res) {
            res->send("Hello! I am VHOST ceres.home.vchrist.at.");
        });
        tlsApp.use(vh3);

        tlsApp.use([] APPLICATION(req, res) {
            res->status(404).send("The requested resource is not found.");
        });

        tlsApp.listen(8088,
                      [instanceName = tlsApp.getConfig().getInstanceName()](const legacy::in6::WebApp::SocketAddress& socketAddress,
                                                                            const core::socket::State& state) {
                          switch (state) {
                              case core::socket::State::OK:
                                  VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                                  break;
                              case core::socket::State::DISABLED:
                                  VLOG(1) << instanceName << " disabled";
                                  break;
                              case core::socket::State::ERROR:
                                  LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                  break;
                              case core::socket::State::FATAL:
                                  LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                  break;
                          }
                      });

        tlsApp.getConfig().setCert("/home/voc/projects/snodec/snode.c/certs/wildcard.home.vchrist.at_-_snode.c_-_server.pem");
        tlsApp.getConfig().setCertKey(
            "/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem");
        tlsApp.getConfig().setCertKeyPassword("snode.c");
    }

    WebApp::start();
}
