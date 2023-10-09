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

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    legacy::in::WebApp legacyApp("legacy");

    legacyApp.get("/", [] APPLICATION(req, res) {
        VLOG(1) << "HTTP GET on "
                << "/";
        if (req.url == "/" || req.url == "/index.html") {
            req.url = "/wstest.html";
        }

        VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req.url;
        res.sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req.url, [&req](int errnum) -> void {
            if (errnum == 0) {
                VLOG(1) << req.url;
            } else {
                VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
            }
        });
    });

    legacyApp.get("/ws", [](Request& req, Response& res) -> void {
        VLOG(1) << "HTTP GET on  legacy /ws";

        std::string uri = req.originalUrl;

        VLOG(2) << "OriginalUri: " << uri;
        VLOG(2) << "Uri: " << req.url;

        VLOG(2) << "Host: " << req.get("host");
        VLOG(2) << "Connection: " << req.get("connection");
        VLOG(2) << "Origin: " << req.get("origin");
        VLOG(2) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
        VLOG(2) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
        VLOG(2) << "sec-websocket-key: " << req.get("sec-websocket-key");
        VLOG(2) << "sec-websocket-version: " << req.get("sec-websocket-version");
        VLOG(2) << "upgrade: " << req.get("upgrade");
        VLOG(2) << "user-agent: " << req.get("user-agent");

        if (!res.upgrade(req)) {
            res.end();
            VLOG(1) << "HTTP upgrade failed";
        }
    });

    legacyApp.listen([](const tls::in::WebApp::SocketAddress& socketAddress, core::socket::State state) -> void {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "legacy: listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "legacy: disabled";
                break;
            case core::socket::State::ERROR:
                VLOG(1) << "legacy: non critical error occurred";
                break;
            case core::socket::State::FATAL:
                VLOG(1) << "legacy: critical error occurred";
                break;
        }
    });

    {
        tls::in::WebApp tlsApp("tls");

        tlsApp.get("/", [] APPLICATION(req, res) {
            if (req.url == "/" || req.url == "/index.html") {
                req.url = "/wstest.html";
            }

            VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req.url;
            res.sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req.url, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.url;
                }
            });
        });

        tlsApp.get("/ws", [](Request& req, Response& res) -> void {
            VLOG(1) << "HTTP GET on  tls /ws";

            std::string uri = req.originalUrl;

            VLOG(2) << "OriginalUri: " << uri;
            VLOG(2) << "Uri: " << req.url;

            VLOG(2) << "Connection: " << req.get("connection");
            VLOG(2) << "Host: " << req.get("host");
            VLOG(2) << "Origin: " << req.get("origin");
            VLOG(2) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
            VLOG(2) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
            VLOG(2) << "sec-websocket-key: " << req.get("sec-websocket-key");
            VLOG(2) << "sec-websocket-version: " << req.get("sec-websocket-version");
            VLOG(2) << "upgrade: " << req.get("upgrade");
            VLOG(2) << "user-agent: " << req.get("user-agent");

            if (!res.upgrade(req)) {
                res.end();
                VLOG(1) << "HTTP upgrade failed";
            }
        });

        tlsApp.listen([](const tls::in::WebApp::SocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "tls: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "tls: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "tls: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "tls: critical error occurred";
                    break;
            }
        });
    }

    return express::WebApp::start();
}

/*
2021-05-14 10:21:39 0000000002:      accept-encoding: gzip, deflate, br
2021-05-14 10:21:39 0000000002:      accept-language: en-us,en;q=0.9,de-at;q=0.8,de-de;q=0.7,de;q=0.6
2021-05-14 10:21:39 0000000002:      cache-control: no-cache
2021-05-14 10:21:39 0000000002:      connection: upgrade, keep-alive
2021-05-14 10:21:39 0000000002:      host: localhost:8080
2021-05-14 10:21:39 0000000002:      origin: file://
2021-05-14 10:21:39 0000000002:      pragma: no-cache
2021-05-14 10:21:39 0000000002:      sec-websocket-extensions: permessage-deflate; client_max_window_bits
2021-05-14 10:21:39 0000000002:      sec-websocket-key: et6vtby1wwyooittpidflw==
2021-05-14 10:21:39 0000000002:      sec-websocket-version: 13
2021-05-14 10:21:39 0000000002:      upgrade: websocket
2021-05-14 10:21:39 0000000002:      user-agent: mozilla/5.0 (x11; linux x86_64) applewebkit/537.36 (khtml, like gecko)
chrome/90.0.4430.212 safari/537.36
*/
