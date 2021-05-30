#include "config.h"
#include "express/legacy/WebApp.h"
#include "express/tls/WebApp.h"
#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/timer/IntervalTimer.h"
#include "web/ws/WSProtocol.h"
#include "web/ws/server/WSContext.h"

#include <cstddef>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <vector>

using namespace express;
using namespace net;

int main(int argc, char* argv[]) {
    SNodeC::init(argc, argv);

    legacy::WebApp legacyApp;

    legacyApp.get("/", [] MIDDLEWARE(req, res, next) {
        if (req.url == "/") {
            req.url = "/wstest.html";
        }

        if (req.url == "/ws") {
            next();
        } else {
            res.sendFile(CMAKE_SOURCE_DIR "apps/html" + req.url, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.url;
                }
            });
        }
    });

    legacyApp.get("/ws", [](Request& req, Response& res) -> void {
        std::string uri = req.originalUrl;

        VLOG(1) << "OriginalUri: " << uri;
        VLOG(1) << "Uri: " << req.url;

        VLOG(1) << "Host: " << req.header("host");
        VLOG(1) << "Connection: " << req.header("connection");
        VLOG(1) << "Origin: " << req.header("origin");
        VLOG(1) << "Sec-WebSocket-Protocol: " << req.header("sec-websocket-protocol");
        VLOG(1) << "sec-web-socket-extensions: " << req.header("sec-websocket-extensions");
        VLOG(1) << "sec-websocket-key: " << req.header("sec-websocket-key");
        VLOG(1) << "sec-websocket-version: " << req.header("sec-websocket-version");
        VLOG(1) << "upgrade: " << req.header("upgrade");
        VLOG(1) << "user-agent: " << req.header("user-agent");

        if (req.header("connection") == "Upgrade") {
            res.upgrade(req);
        } else {
            res.sendStatus(404);
        }
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080" << std::endl;
        }
    });

    {
        tls::WebApp tlsApp({{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}});

        tlsApp.get("/", [] MIDDLEWARE(req, res, next) {
            if (req.url == "/") {
                req.url = "/wstest.html";
            }

            if (req.url == "/ws") {
                next();
            } else {
                VLOG(0) << CMAKE_SOURCE_DIR "apps/html";
                res.sendFile(CMAKE_SOURCE_DIR "apps/html" + req.url, [&req](int ret) -> void {
                    if (ret != 0) {
                        PLOG(ERROR) << req.url;
                    }
                });
            }
        });

        tlsApp.get("/ws", [](Request& req, Response& res) -> void {
            std::string uri = req.originalUrl;

            VLOG(1) << "OriginalUri: " << uri;
            VLOG(1) << "Uri: " << req.url;

            VLOG(1) << "Connection: " << req.header("connection");
            VLOG(1) << "Host: " << req.header("host");
            VLOG(1) << "Origin: " << req.header("origin");
            VLOG(1) << "Sec-WebSocket-Protocol: " << req.header("sec-websocket-protocol");
            VLOG(1) << "sec-web-socket-extensions: " << req.header("sec-websocket-extensions");
            VLOG(1) << "sec-websocket-key: " << req.header("sec-websocket-key");
            VLOG(1) << "sec-websocket-version: " << req.header("sec-websocket-version");
            VLOG(1) << "upgrade: " << req.header("upgrade");
            VLOG(1) << "user-agent: " << req.header("user-agent");

            if (req.header("connection") == "Upgrade") {
                res.upgrade(req);
            } else {
                res.sendStatus(404);
            }
        });

        tlsApp.listen(8088, [](int err) -> void {
            if (err != 0) {
                perror("Listen");
            } else {
                std::cout << "snode.c listening on port 8088" << std::endl;
            }
        });
    }

    return SNodeC::start();
}

/*
2021-05-14 10:21:39 0000000002:      accept-encoding: gzip, deflate, br
2021-05-14 10:21:39 0000000002:      accept-language: en-us,en;q=0.9,de-at;q=0.8,de-de;q=0.7,de;q=0.6
2021-05-14 10:21:39 0000000002:      cache-control: no-cache
2021-05-14 10:21:39 0000000002:      connection: upgrade
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
