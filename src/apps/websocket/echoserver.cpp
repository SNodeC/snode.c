/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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
#include "express/middleware/VerboseRequest.h"
#include "express/tls/in/WebApp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstring>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    using LegacyWebApp = express::legacy::in::WebApp;
    using Request = LegacyWebApp::Request;
    using Response = LegacyWebApp::Response;
    using SocketAddress = LegacyWebApp::SocketAddress;

    const LegacyWebApp legacyApp("legacy");

    legacyApp.use(express::middleware::VerboseRequest());

    legacyApp.get("/", [] APPLICATION(req, res) {
        VLOG(1) << "HTTP GET on "
                << "/";
        if (req->url == "/" || req->url == "/index.html") {
            req->url = "/wstest.html";
        }

        VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
        res->sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req->url, [req](int errnum) {
            if (errnum == 0) {
                VLOG(1) << req->url;
            } else {
                VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
            }
        });
    });

    legacyApp.get("/ws", [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
        VLOG(1) << "HTTP GET on legacy /ws";

        const std::string uri = req->originalUrl;

        VLOG(2) << "OriginalUri: " << uri;
        VLOG(2) << "Uri: " << req->url;

        VLOG(2) << "Host: " << req->get("host");
        VLOG(2) << "Connection: " << req->get("connection");
        VLOG(2) << "Origin: " << req->get("origin");
        VLOG(2) << "Sec-WebSocket-Protocol: " << req->get("sec-websocket-protocol");
        VLOG(2) << "sec-web-socket-extensions: " << req->get("sec-websocket-extensions");
        VLOG(2) << "sec-websocket-key: " << req->get("sec-websocket-key");
        VLOG(2) << "sec-websocket-version: " << req->get("sec-websocket-version");
        VLOG(2) << "upgrade: " << req->get("upgrade");
        VLOG(2) << "user-agent: " << req->get("user-agent");

        if (req->get("sec-websocket-protocol").find("echo") != std::string::npos) {
            res->upgrade(req, [req, res](const std::string& name) {
                if (!name.empty()) {
                    VLOG(1) << "Successful upgrade to '" << name << "' from options: " << req->get("upgrade");
                } else {
                    VLOG(1) << "Can not upgrade to any of '" << req->get("upgrade") << "'";
                }
                res->end();
            });
        } else {
            res->sendStatus(404);
        }
    });

    legacyApp.listen(
        [instanceName = legacyApp.getConfig().getInstanceName()](const SocketAddress& socketAddress, const core::socket::State& state) {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << instanceName << " disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                    break;
            }
        });

    {
        using TlsWebApp = express::tls::in::WebApp;
        using Request = TlsWebApp::Request;
        using Response = TlsWebApp::Response;
        using SocketAddress = TlsWebApp::SocketAddress;

        const TlsWebApp tlsApp("tls");

        tlsApp.use(express::middleware::VerboseRequest());

        tlsApp.get("/", [] APPLICATION(req, res) {
            if (req->url == "/" || req->url == "/index.html") {
                req->url = "/wstest.html";
            }

            VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
            res->sendFile(CMAKE_CURRENT_SOURCE_DIR "/html" + req->url, [req](int ret) {
                if (ret != 0) {
                    PLOG(ERROR) << req->url;
                }
            });
        });

        tlsApp.get("/ws", [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
            VLOG(1) << "HTTP GET on tls /ws";

            const std::string uri = req->originalUrl;

            VLOG(2) << "OriginalUri: " << uri;
            VLOG(2) << "Uri: " << req->url;

            VLOG(2) << "Connection: " << req->get("connection");
            VLOG(2) << "Host: " << req->get("host");
            VLOG(2) << "Origin: " << req->get("origin");
            VLOG(2) << "Sec-WebSocket-Protocol: " << req->get("sec-websocket-protocol");
            VLOG(2) << "sec-web-socket-extensions: " << req->get("sec-websocket-extensions");
            VLOG(2) << "sec-websocket-key: " << req->get("sec-websocket-key");
            VLOG(2) << "sec-websocket-version: " << req->get("sec-websocket-version");
            VLOG(2) << "upgrade: " << req->get("upgrade");
            VLOG(2) << "user-agent: " << req->get("user-agent");

            if (req->get("sec-websocket-protocol").find("echo") != std::string::npos) {
                res->upgrade(req, [req, res](const std::string& name) {
                    if (!name.empty()) {
                        VLOG(1) << "Successful upgrade to '" << name << "' from options: " << req->get("upgrade");
                    } else {
                        VLOG(1) << "Can not upgrade to any of '" << req->get("upgrade") << "'";
                    }
                    res->end();
                });
            } else {
                res->sendStatus(404);
            }
        });

        tlsApp.listen(
            [instanceName = tlsApp.getConfig().getInstanceName()](const SocketAddress& socketAddress, const core::socket::State& state) {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << instanceName << " disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
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
