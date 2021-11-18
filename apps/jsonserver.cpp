/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021  Volker Christian <me@vchrist.at>
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

#include "express/legacy/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "log/Logger.h"
#include "net/socket/ip/socket/ipv4/InetAddress.h" // for InetAddress

#include <string> // for allocator, opera...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    legacy::WebApp legacyApp;

    legacyApp.use(middleware::JsonMiddleware());

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080 " << std::to_string(err);
        } else {
            VLOG(0) << "jsonserver.c listening on port 8080 for legacy connections";
        }
    });

    legacyApp.post("/index.html", [] APPLICATION(req, res) {
        std::string jsonString = "";

        req.getAttribute<json>(
            [&jsonString](json& json) -> void {
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

    legacyApp.onConnect([](const legacy::WebApp::SocketAddress& localAddress, const legacy::WebApp::SocketAddress& remoteAddress) -> void {
        VLOG(0) << "OnConnect:";

        VLOG(0) << "\tServer: (" + localAddress.address() + ") " + localAddress.toString();
        VLOG(0) << "\tClient: (" + remoteAddress.address() + ") " + remoteAddress.toString();
    });

    legacyApp.onDisconnect([](legacy::WebApp::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";

        VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
        VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " + socketConnection->getRemoteAddress().toString();
    });

    return WebApp::start();
}
