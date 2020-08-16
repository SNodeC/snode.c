/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
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
#include "tls/WebApp.h"

#include <easylogging++.h>
#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_server.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"

#define SERVERROOT "/home/voc/projects/ServerVoc/build/html/"

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    Router router;
    router
        .use(
            "/",
            MIDDLEWARE(req, res, next) {
                res.set("Connection", "Keep-Alive");
                next();
            })
        .get(
            "/",
            APPLICATION(req, res) {
                VLOG(0) << "URL: " + req.url;
                if (req.originalUrl == "/") {
                    res.redirect(308, "/index.html");
                } else if (req.url == "/end") {
                    res.send("Bye, bye!\n");
                    WebApp::stop();
                } else {
                    res.sendFile("/home/voc/projects/ServerVoc/build/html/" + req.url, [&req](int ret) -> void {
                        if (ret != 0) {
                            PLOG(ERROR) << req.url;
                        }
                    });
                }
            })
        .get(
            "/search", APPLICATION(req, res) {
                VLOG(0) << "URL: " + req.url;
                res.sendFile("/home/voc/projects/ServerVoc/build/html/" + req.url, [&req](int ret) -> void {
                    if (ret != 0) {
                        PLOG(ERROR) << req.url;
                    }
                });
            });

    legacy::WebApp legacyApp;
    legacyApp.use("/", router);

    tls::WebApp tlsApp(CERTF, KEYF, KEYFPASS);
    tlsApp.use("/", router);

    tlsApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    WebApp::start();

    return 0;
}
