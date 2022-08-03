/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "config.h"
#include "express/legacy/in/WebApp.h"
#include "express/tls/in/WebApp.h"
#include "log/Logger.h"
#include "web/http/http_utils.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    legacy::in::WebApp legacyApp("legacy");

    legacyApp.get("/", [] APPLICATION(req, res) {
        if (req.url == "/" || req.url == "/index.html") {
            req.url = "/wstest.html";
        }

        VLOG(0) << CMAKE_CURRENT_SOURCE_DIR "/html" + req.url;
        res.sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req.url, [&req](int ret) -> void {
            if (ret != 0) {
                PLOG(ERROR) << req.url;
            }
        });
    });

    legacyApp.get("/ws", [](Request& req, Response& res) -> void {
        std::string uri = req.originalUrl;

        VLOG(1) << "OriginalUri: " << uri;
        VLOG(1) << "Uri: " << req.url;

        VLOG(1) << "Host: " << req.get("host");
        VLOG(1) << "Connection: " << req.get("connection");
        VLOG(1) << "Origin: " << req.get("origin");
        VLOG(1) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
        VLOG(1) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
        VLOG(1) << "sec-websocket-key: " << req.get("sec-websocket-key");
        VLOG(1) << "sec-websocket-version: " << req.get("sec-websocket-version");
        VLOG(1) << "upgrade: " << req.get("upgrade");
        VLOG(1) << "user-agent: " << req.get("user-agent");

        if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
            res.upgrade(req);
        } else {
            res.sendStatus(404);
        }
    });

    legacyApp.listen([](const tls::in::WebApp::SocketAddress& socketAddress, int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        } else {
            VLOG(0) << "wsechoserver listening on " << socketAddress.toString();
        }
    });

    {
        tls::in::WebApp tlsApp("tls", {{"CertChain", SERVERCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}});

        tlsApp.get("/", [] APPLICATION(req, res) {
            if (req.url == "/" || req.url == "/index.html") {
                req.url = "/wstest.html";
            }

            VLOG(0) << CMAKE_CURRENT_SOURCE_DIR "/html" + req.url;
            res.sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req.url, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.url;
                }
            });
        });

        tlsApp.get("/ws", [](Request& req, Response& res) -> void {
            std::string uri = req.originalUrl;

            VLOG(1) << "OriginalUri: " << uri;
            VLOG(1) << "Uri: " << req.url;

            VLOG(1) << "Connection: " << req.get("connection");
            VLOG(1) << "Host: " << req.get("host");
            VLOG(1) << "Origin: " << req.get("origin");
            VLOG(1) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
            VLOG(1) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
            VLOG(1) << "sec-websocket-key: " << req.get("sec-websocket-key");
            VLOG(1) << "sec-websocket-version: " << req.get("sec-websocket-version");
            VLOG(1) << "upgrade: " << req.get("upgrade");
            VLOG(1) << "user-agent: " << req.get("user-agent");

            if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
                if (req.get("Sec-WebSocket-Protocol") == "echo") {
                    res.upgrade(req);
                } else {
                    res.sendStatus(404);
                }
            } else {
                res.sendStatus(404);
            }
        });

        tlsApp.listen([](const tls::in::WebApp::SocketAddress& socketAddress, int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            } else {
                VLOG(0) << "wsechoserver listening on " << socketAddress.toString();
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
