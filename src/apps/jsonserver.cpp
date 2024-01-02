/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023  Volker Christian <me@vchrist.at>
 * Json Middleware 2020, 2021 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
 * Github <MarleneMayr><moseranna><MatteoMatteoMatteo><peregrin-tuk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "log/Logger.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// using namespace express;

int main(int argc, char* argv[]) {
    using WebApp = express::legacy::in::WebApp;

    WebApp::init(argc, argv);

    //    el::Loggers::setVModules("jsonserver*=0");

    using SocketAddress = WebApp::SocketAddress;

    WebApp legacyApp("legacy-jsonserver");

    legacyApp.use(express::middleware::JsonMiddleware());

    legacyApp.listen(8080,
                     [instanceName = legacyApp.getConfig().getInstanceName()](const SocketAddress& socketAddress,
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

    legacyApp.post("/index.html", [] APPLICATION(req, res) {
        std::string jsonString;

        req.getAttribute<nlohmann::json>(
            [&jsonString](nlohmann::json& json) -> void {
                jsonString = json.dump(4);
                VLOG(0) << "Application received body: " << jsonString;
            },
            [](const std::string& key) -> void {
                VLOG(0) << key << " attribute not found";
            });

        res.send(jsonString);
    });

    legacyApp.post([] APPLICATION(req, res) {
        res.send("Wrong Url");
    });

    return WebApp::start();
}
