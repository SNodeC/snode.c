/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "config.h" // just for this example app
#include "express/legacy/WebApp.h"
#include "http/websocket/WSServerContext.h"
#include "net/timer/IntervalTimer.h"
#include "net/timer/SingleshotTimer.h"

#include <cstring>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;
using namespace net::timer;

int timerApp() {
    [[maybe_unused]] const Timer& tick = Timer::continousTimer(
        [](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
            static int i = 0;
            std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;
        },
        {0, 500000},
        "Tick");

    Timer& tack = Timer::continousTimer(
        [](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
            static int i = 0;
            std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;
        },
        {1, 100000},
        "Tack");

    bool canceled = false;

    legacy::WebApp app;

    app.get("/", [&canceled, &tack](Request& req, Response& res) -> void {
        std::string uri = req.originalUrl;

        if (uri == "/") {
            res.redirect("/index.html");
        } else {
            std::cout << "OriginalUri: " << uri << std::endl;
            std::cout << "Uri: " << req.url << std::endl;

            std::cout << "RCookie: " << req.cookie("doxygen_width") << std::endl;
            std::cout << "RCookie: " << req.cookie("test") << std::endl;

            //            std::cout << "RHeader: " << req.header("Content-Length") << std::endl;
            std::cout << "RHeader: " << req.header("Accept") << std::endl;

            std::cout << "If-Modified: " << req.header("If-Modified-Since") << std::endl;

            std::cout << "RQuery: " << req.query("Hallo") << std::endl;

            res.cookie("Test", "me", {{"Max-Age", "3600"}});

            //            res.set("Connection", "close");
            res.sendFile("/home/voc/projects/ServerVoc/docs/html" + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    perror(uri.c_str());
                    //                    std::cout << "Error: " << ret << ", " << uri << std::endl;
                }
            });

            if (!canceled) {
                tack.cancel();
                canceled = true;
                //                app.destroy();
            }
            //            Multiplexer::stop();
        }
    });

    app.get("/search/", [&](Request& req, Response& res) -> void {
        //                  res.set({{"Content-Length", "7"}});

        std::string host = req.header("Host");

        //                std::cout << "Host: " << host << std::endl;

        std::string uri = req.originalUrl;

        //                std::cout << "RHeader: " << req.header("Accept") << std::endl;

        std::cout << "OriginalUri: " << uri << std::endl;
        std::cout << "Uri: " << req.url << std::endl;

        if (uri == "/") {
            res.redirect("/index.html");
        } else {
            //                    std::cout << uri << std::endl;

            if (req.bodyLength() != 0) {
                std::cout << "Body: " << req.body << std::endl;
            }

            res.sendFile("/home/voc/projects/ServerVoc/docs/html" + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    std::cerr << uri << ": " << strerror(ret) << std::endl;
                }
            });
        }
        res.upgrade(new http::websocket::WSServerContext());
    });

    app.listen(8080, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080" << std::endl;
        }
    });

    WebApp::start();

    return 0;
}

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    return timerApp();
}
