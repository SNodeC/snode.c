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

#include "express/legacy/in/WebApp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdlib>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

static std::map<std::string, std::string> jalousien = {{"kueche", "kueche"},
                                                       {"strasse", "strasse"},
                                                       {"esstisch", "esstisch"},
                                                       {"balkon", "balkon"},
                                                       {"schlafzimmer", "schlafzimmer"},
                                                       {"arbeitszimmer", "arbeitszimmer"},
                                                       {"comfort", "komfort"},
                                                       {"all", "alle"}};

static std::map<std::string, std::string> actions = {{"open", "up"}, {"close", "down"}, {"stop", "stop"}};

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    legacy::in::WebApp webApp("werema");

    //    tls::WebApp wa;

    webApp.get("/jalousien/:id", [] APPLICATION(req, res) {
        VLOG(0) << "Param: " << req->param("id");
        VLOG(0) << "Qurey: " << req->query("action");

        std::string arguments = "aircontrol -t " + jalousien[req->param("id")] + "_" + actions[req->query("action")];

        int ret = system(arguments.c_str());
        ret = (ret >> 8) & 0xFF;
        /* ret:
               Bits 15-8 = Exit code.
               Bit     7 = 1 if a core dump was produced.
               Bits  6-0 = Signal number that killed the process.
        */

        switch (ret) {
            case 0:
                res->status(200).send("OK: " + arguments);
                break;
            case 127:
                res->status(404).send("ERROR not found: " + arguments);
                break;
        }
    });

    webApp.use([] APPLICATION(req, res) {
        res->status(404).send("No Jalousie specified");
    });

    webApp.listen(8080,
                  [instanceName = webApp.getConfig().getInstanceName()](const legacy::in::WebApp::SocketAddress& socketAddress,
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

    return WebApp::start();
}
