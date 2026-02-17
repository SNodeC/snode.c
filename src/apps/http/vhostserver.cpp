/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "ConfigWWW.h"
#include "express/legacy/in6/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "express/tls/in6/WebApp.h"
#include "net/config/ConfigInstanceAPI.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router getRouter(const std::string& webRoot) {
    const Router router;

    router.use(middleware::StaticMiddleware(webRoot));

    return router;
}

int main(int argc, char* argv[]) {
    utils::Config::addInstance<instance::ConfigWWW>();

    WebApp::init(argc, argv);

    {
        const legacy::in6::WebApp legacyApp("legacy");

        const Router& vh1 = middleware::VHost("localhost:8080");
        vh1.use(middleware::StaticMiddleware(utils::Config::getInstance<instance::ConfigWWW>()->getHtmlRoot()));
        legacyApp.use(vh1);

        const Router& vh2 = middleware::VHost("jupiter.home.vchrist.at");
        vh2.get("/",
                [] MIDDLEWARE(req, res, next) {
                    if (req->query("how").empty()) {
                        next();
                    } else if (req->query("how") == "route") {
                        next("route");
                    } else if (req->query("how") == "router") {
                        next("router");
                    }
                })
            .get([] APPLICATION(req, res) {
                res->send("Hello! I am VHOST jupiter.home.vchrist.at.");
            });
        legacyApp.use(vh2);

        const Router& vh3 = middleware::VHost("atlas.home.vchrist.at:8080");
        vh3.get("/",
                [] MIDDLEWARE(req, res, next) {
                    if (req->query("how").empty()) {
                        next();
                    } else if (req->query("how") == "route") {
                        next("route");
                    } else if (req->query("how") == "router") {
                        next("router");
                    }
                })
            .get([] APPLICATION(req, res) {
                res->send("Hello! I am VHOST atlas.home.vchrist.at.");
            });
        vh3.get("/", [] APPLICATION(req, res) {
            res->send("Hello, next route on atlas.home.vchrist.at");
        });
        vh3.get("/test", [] APPLICATION(req, res) {
            res->send("Test Route on VHOST atlas.home.vchrist.at.");
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
        vh1.use(middleware::StaticMiddleware(utils::Config::getInstance<instance::ConfigWWW>()->getHtmlRoot()));
        tlsApp.use(vh1);

        const Router& vh2 = middleware::VHost("atlas.home.vchrist.at:8088");
        vh2.get("/",
                [] MIDDLEWARE(req, res, next) {
                    if (req->query("how").empty()) {
                        next();
                    } else if (req->query("how") == "route") {
                        next("route");
                    } else if (req->query("how") == "router") {
                        next("router");
                    }
                })
            .get([] APPLICATION(req, res) {
                res->send("Hello! I am VHOST atlas.home.vchrist.at.");
            });
        tlsApp.use(vh2);

        const Router& vh3 = middleware::VHost("ceres.home.vchrist.at:8088");
        vh3.get("/",
                [] MIDDLEWARE(req, res, next) {
                    if (req->query("how").empty()) {
                        next();
                    } else if (req->query("how") == "route") {
                        next("route");
                    } else if (req->query("how") == "router") {
                        next("router");
                    }
                })
            .get([] APPLICATION(req, res) {
                res->send("Hello! I am VHOST ceres.home.vchrist.at.");
            });
        vh3.get("/", [] APPLICATION(req, res) {
            res->send("Hello, next route on ceres.home.vchrist.at");
        });
        vh3.get("/test", [] APPLICATION(req, res) {
            res->send("Test Route on VHOST ceres.home.vchrist.at.");
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
