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

#include "config.h" // just for this example app
#include "express/legacy/WebApp.h"
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
            std::cout << "RCookie: " << req.cookie("doxygen_width") << std::endl;
            std::cout << "RCookie: " << req.cookie("Test") << std::endl;

            //            std::cout << "RHeader: " << req.header("Content-Length") << std::endl;
            std::cout << "RHeader: " << req.header("Accept") << std::endl;

            std::cout << "If-Modified: " << req.header("If-Modified-Since") << std::endl;

            std::cout << "RQuery: " << req.query("Hallo") << std::endl;

            res.cookie("Test", "me", {{"Max-Age", "3600"}});

            //            res.set("Connection", "close");
            res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri](int ret) -> void {
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

            res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    std::cerr << uri << ": " << strerror(ret) << std::endl;
                }
            });
        }
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

int simpleWebserver() {
    legacy::WebApp app;

    Router router;

    router.get("/search/", []([[maybe_unused]] Request& req, [[maybe_unused]] Response& res, const std::function<void(void)>& next) {
        std::cout << "Route 3" << std::endl;
        next();
    });

    router.get("/search/", [&](Request& req, Response& res) -> void {
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

            res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    std::cerr << uri << ": " << strerror(ret) << std::endl;
                }
            });
        }
    });

    app.use("/", []([[maybe_unused]] Request& req, [[maybe_unused]] Response& res, const std::function<void(void)>& next) {
        std::cout << "Route 1" << std::endl;
        next();
    });

    app.use("/", []([[maybe_unused]] Request& req, [[maybe_unused]] Response& res, const std::function<void(void)>& next) {
        std::cout << "Route 2" << std::endl;
        next();
    });

    app.get("/", [&](Request& req, Response& res) -> void {
        res.cookie("Test", "me", {{"Max-Age", "3600"}});

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

            res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    std::cerr << uri << ": " << strerror(ret) << std::endl;
                }
            });
        }
    });

    app.get("/", router);

    /*
    app.get("/search",
            [&] (const Request& req, const Response& res) -> void {

                std::string host = req.header("Host");

                //                std::cout << "Host: " << host << std::endl;

                std::string uri = req.originalUrl;

                //                std::cout << "RHeader: " << req.header("Accept") << std::endl;

                std::cout << "Uri: " << uri << std::endl;

                if (uri == "/") {
                    res.redirect("/index.html");
                } else {
                    //                    std::cout << uri << std::endl;

                    if (req.bodySize() != 0) {
                        std::cout << "Body: " << req.body << std::endl;
                    }

                    res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri] (int ret) -> void {
                        if (ret != 0) {
                            std::cerr << uri << ": " << strerror(ret) << std::endl;
                        }
                    });
                }
            });*/

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

int testPost() {
    legacy::WebApp app;

    app.get("/", [&]([[maybe_unused]] Request& req, Response& res) -> void {
        res.send("<html>"
                 "<head>"
                 "<style>"
                 "main {"
                 "min-height: 30em;"
                 "padding: 3em;"
                 "background-image: repeating-radial-gradient( circle at 0 0, #fff, #ddd 50px);"
                 "}"
                 "input[type=\"file\"] {"
                 "display: block;"
                 "margin: 2em;"
                 "padding: 2em;"
                 "border: 1px dotted;"
                 "}"
                 "</style>"
                 "</head>"
                 "<body>"
                 "<h1>Datei-Upload mit input type=\"file\"</h1>"
                 "<main>"
                 "<h2>Schicken Sie uns was Schickes!</h2>"
                 "<form method=\"post\" enctype=\"multipart/form-data\">"
                 "<label> Wählen Sie eine Textdatei (*.txt, *.html usw.) von Ihrem Rechner aus."
                 "<input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"
                 "</label>"
                 "<button>… und ab geht die Post!</button>"
                 "</form>"
                 "</main>"
                 "</body>"
                 "</html>");
    });

    app.post("/", [&](Request& req, Response& res) -> void {
        std::cout << "Content-Type: " << req.header("Content-Type") << std::endl;
        std::cout << "Content-Length: " << req.header("Content-Length") << std::endl;
        char* body = new char[std::stoul(req.header("Content-Length")) + 1];
        memcpy(body, req.body, std::stoul(req.header("Content-Length")));
        body[std::stoi(req.header("Content-Length"))] = 0;

        std::cout << "Body: " << std::endl;
        std::cout << body << std::endl;
        res.send("<html>"
                 "<body>"
                 "<h1>Thank you</h1>"
                 "</body>"
                 "</html>");

        delete[] body;
    });

    app.listen(8080, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080" << std::endl;
        }
    });

    return WebApp::start();
}

int main(int argc, char** argv) {
    const char str[] = "ÅÄÖ";
    std::cout << sizeof(str) << ' ' << strlen(str) << ' ' << sizeof("Åäö") << ' ' << strlen("åäö") << std::endl;

    std::string s = "ÅÄÖ";
    std::cout << "String: \"" << s << "\" Length: " << s.length() << std::endl;

    std::cout << "String: \"" << s.c_str() << "\" Length: " << strlen(s.c_str()) << std::endl;

    std::string s1 = str;
    std::cout << "String: \"" << s1 << "\" Length: " << s1.length() << std::endl;

    std::cout << "String: \"" << s1.c_str() << "\" Length: " << strlen(s1.c_str()) << std::endl;

    WebApp::init(argc, argv);

    return timerApp();
}
