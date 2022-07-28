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
#include "core/timer/Timer.h"
#include "express/legacy/in/WebApp.h"
#include "log/Logger.h"

#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <typeinfo>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace core::timer;

class File {
public:
    File()
        : flags(0) {
    }

    explicit File(int flags) {
        this->flags |= flags;
    }

    void setFlags(int flags) {
        this->flags |= flags;

        if ((this->flags & FL_RDONLY) && !(this->flags & FL_WRONLY)) {
            rflags = O_RDONLY;
        } else if (!(this->flags & FL_RDONLY) && (this->flags & FL_WRONLY)) {
            rflags = O_WRONLY;
        } else if ((this->flags & FL_RDONLY) && (this->flags & FL_WRONLY)) {
            rflags = O_RDWR;
        }
    }

    void addFlags(int flags) {
        this->flags |= flags;
    }

    int flags = 0;
    int rflags = 0;

    int FL_RDONLY = 1;
    int FL_WRONLY = 2;
};

class FileReader : public virtual File {
public:
    FileReader() {
        File::setFlags(FL_RDONLY);
    }
};

class FileWriter : public virtual File {
public:
    FileWriter() {
        File::setFlags(FL_WRONLY);
    }
};

class FileIO
    : public FileReader
    , public FileWriter {
public:
    FileIO() {
    }
};

int timerApp();

int timerApp() {
    [[maybe_unused]] const Timer tick = Timer::intervalTimer(
        [](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
            static int i = 0;
            std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;
        },
        0.5,
        "Tick");

    Timer tack = Timer::intervalTimer(
        [](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
            static int i = 0;
            std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;
            stop();
        },
        1.1,
        "Tack");

    bool canceled = false;

    express::legacy::in::WebApp app("testapp");

    app.get("/", [&canceled, &tack](express::Request& req, express::Response& res) -> void {
        std::string uri = req.originalUrl;

        if (uri == "/") {
            res.redirect("/index.html");
        } else {
            std::cout << "OriginalUri: " << uri << std::endl;
            std::cout << "Uri: " << req.url << std::endl;

            std::cout << "RCookie: " << req.cookie("doxygen_width") << std::endl;
            std::cout << "RCookie: " << req.cookie("test") << std::endl;

            //            std::cout << "RHeader: " << req.header("Content-Length") << std::endl;
            std::cout << "RHeader: " << req.get("Accept") << std::endl;

            std::cout << "If-Modified: " << req.get("If-Modified-Since") << std::endl;

            std::cout << "RQuery: " << req.query("Hallo") << std::endl;

            res.cookie("Test", "me", {{"Max-Age", "3600"}});

            //            res.set("Connection", "close");
            res.sendFile(SERVERROOT + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    perror(uri.c_str());
                    //                    std::cout << "Error: " << ret << ", " << uri << std::endl;
                }
            });

            if (!canceled) {
                VLOG(0) << "----------------------------- timer cancel";
                tack.cancel();
                canceled = true;
                //                app.destroy();
            }
            //            Multiplexer::stop();
        }
    });

    app.get("/search/", [&](express::Request& req, express::Response& res) -> void {
        //                  res.set({{"Content-Length", "7"}});

        std::string host = req.get("Host");

        //                std::cout << "Host: " << host << std::endl;

        std::string uri = req.originalUrl;

        //                std::cout << "RHeader: " << req.header("Accept") << std::endl;

        std::cout << "OriginalUri: " << uri << std::endl;
        std::cout << "Uri: " << req.url << std::endl;

        if (uri == "/") {
            res.redirect("/index.html");
        } else {
            //                    std::cout << uri << std::endl;

            if (req.body.size() > 0) {
                req.body.push_back(0);
                std::cout << "Body: " << req.body.data() << std::endl;
            }

            res.sendFile(SERVERROOT + uri, [uri](int ret) -> void {
                if (ret != 0) {
                    std::cerr << uri << ": " << strerror(ret) << std::endl;
                }
            });
        }
        res.upgrade(req);
    });

    app.listen(8080, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening: " << socketAddress.toString();
        }
    });

    return express::WebApp::start();
}

class Test {
public:
    void filter(const std::function<void(int)>& function) {
        this->function = function;
    }

    void callIt() {
        function(3);
    }

private:
    std::function<void(int)> function;
};

void g() {
}

void g(const std::function<void(express::Request&, express::Response&)>& arg) {
    VLOG(0) << "Arg type 1: " << typeid(arg).name();
}

void g(const std::function<void(express::Request&, express::Response&, express::Next&& next)>& arg) {
    VLOG(0) << "Arg type 2: " << typeid(arg).name();
}

template <typename... Ts>
void g(const std::function<void(express::Request&, express::Response&)>& arg, Ts... args) {
    VLOG(0) << "Rec Arg type 1: " << typeid(arg).name();

    g(args...);
}

template <typename... Ts>
void g(const std::function<void(express::Request&, express::Response&, express::Next&& next)>& arg, Ts... args) {
    VLOG(0) << "Rec Arg type 2: " << typeid(arg).name();

    g(args...);
}

int main(int argc, char** argv) {
    express::WebApp::init(argc, argv);

    g(
        []([[maybe_unused]] express::Request& req, [[maybe_unused]] express::Response& res, [[maybe_unused]] express::Next&& next) -> void {
            VLOG(0) << "hihih";
        },
        []([[maybe_unused]] express::Request& req, [[maybe_unused]] express::Response& res) -> void {
            VLOG(0) << "hihih";
        },
        []([[maybe_unused]] express::Request& req, [[maybe_unused]] express::Response& res) -> void {
            VLOG(0) << "hihih";
        });

    File test;
    FileReader test1;
    FileWriter test2;
    FileIO test3;

    std::cout << "File: " << test.rflags << std::endl;
    std::cout << "FileReader: " << test1.rflags << " : " << O_RDONLY << std::endl;
    std::cout << "FileWriter: " << test2.rflags << " : " << O_WRONLY << std::endl;
    std::cout << "FileIO: " << test3.rflags << " : " << O_RDWR << std::endl;

    return timerApp();
}
