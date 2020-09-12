/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "Logger.h"
#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <cstring>
#include <easylogging++.h>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_server.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"

using namespace express;

int testPost() {
    legacy::WebApp legacyApp;

    legacyApp.get("/", [] APPLICATION(req, res) {
        res.send("<html>"
                 "    <head>"
                 "        <style>"
                 "            main {"
                 "                min-height: 30em;"
                 "                padding: 3em;"
                 "                background-image: repeating-radial-gradient( circle at 0 0, #fff, #ddd 50px);"
                 "            }"
                 "            input[type=\"file\"] {"
                 "                display: block;"
                 "                margin: 2em;"
                 "                padding: 2em;"
                 "                border: 1px dotted;"
                 "            }"
                 "        </style>"
                 "    </head>"
                 "    <body>"
                 "        <h1>Datei-Upload mit input type=\"file\"</h1>"
                 "        <main>"
                 "            <h2>Schicken Sie uns was Schickes!</h2>"
                 "            <form method=\"post\" enctype=\"multipart/form-data\">"
                 "                <label> Wählen Sie eine Textdatei (*.txt, *.html usw.) von Ihrem Rechner aus."
                 "                    <input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"
                 "                </label>"
                 "                <button>… und ab geht die Post!</button>"
                 "            </form>"
                 "        </main>"
                 "    </body>"
                 "</html>");
    });

    legacyApp.post("/", [] APPLICATION(req, res) {
        std::cout << "Content-Type: " << req.header("Content-Type") << std::endl;
        std::cout << "Content-Length: " << req.header("Content-Length") << std::endl;
        char* body = new char[std::stoul(req.header("Content-Length")) + 1];
        memcpy(body, req.body, std::stoul(req.header("Content-Length")));
        body[std::stoi(req.header("Content-Length"))] = 0;

        std::cout << "Body: " << std::endl;
        std::cout << body << std::endl;
        res.send("<html>"
                 "    <body>"
                 "        <h1>Thank you</h1>"
                 "    </body>"
                 "</html>");

        delete[] body;
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    //    tls::WebApp tlsWebApp(CERTF, KEYF, KEYFPASS);
    tls::WebApp tlsWebApp({{"certChain", CERTF}, {"keyPEM", KEYF}, {"password", KEYFPASS}});
    tlsWebApp.use(legacyApp);

    tlsWebApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    WebApp::start();

    return 0;
}

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    return testPost();
}
