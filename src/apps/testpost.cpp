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

#include "express/legacy/in/WebApp.h"
#include "express/tls/in/WebApp.h"
#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    express::legacy::in::WebApp legacyApp("legacy-testpost");

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
        VLOG(0) << "Content-Type: " << req.get("Content-Type");
        VLOG(0) << "Content-Length: " << req.get("Content-Length");

        req.body.push_back(0);
        VLOG(0) << req.body.data();

        res.send("<html>"
                 "    <body>"
                 "        <h1>Thank you</h1>"
                 "    </body>"
                 "</html>");
    });

    express::tls::in::WebApp tlsApp("tls-testpost");
    tlsApp.use(legacyApp);

    legacyApp.listen(8080, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    tlsApp.listen(8088, [](const express::tls::in::WebApp::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    return express::WebApp::start();
}
