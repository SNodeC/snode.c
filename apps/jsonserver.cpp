/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 * Json Middleware 2020 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
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

#include "legacy/WebApp.h"
#include "middleware/JsonMiddleware.h"
#include "middleware/StaticMiddleware.h"
#include "tls/WebApp.h"

#include <easylogging++.h>
#include <nlohmann/json.hpp>
#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/student/nds/snode.c/certs/snode.c_-_server.pem"
#define KEYF "/home/student/nds/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"
#define CLIENTCAFILE "/home/student/nds/snode.c/certs/Volker_Christian_-_Root_CA.crt"

#define SERVERROOT "/home/student/nds/snode.c/doc/html"

using namespace express;
using json = nlohmann::json;

int main(int argc, char** argv) {
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

    legacyApp.post("/", [] APPLICATION(req, res) {
        std::string jsonString = "";

        req.getAttribute<json>(
            [&jsonString](json& j) -> void {
                jsonString = j.dump(4);
                VLOG(0) << "Application received body: " << jsonString;
            },
            [](const std::string& key) -> void {
                VLOG(0) << key << " attribute not found";
            });

        res.send(jsonString);
    });

    legacyApp.onConnect([](net::socket::tcp::legacy::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnConnect:";
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                       "):" + std::to_string(socketConnection->getRemoteAddress().port());
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                       "):" + std::to_string(socketConnection->getLocalAddress().port());
    });

    legacyApp.onDisconnect([](net::socket::tcp::legacy::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect:";
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                       "):" + std::to_string(socketConnection->getRemoteAddress().port());
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                       "):" + std::to_string(socketConnection->getLocalAddress().port());
    });

    return WebApp::start();
}
