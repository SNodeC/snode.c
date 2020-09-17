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

#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventLoop.h"
#include "WebApp.h"

namespace express {

    bool WebApp::initialized{false};

    WebApp::WebApp() {
        if (!initialized) {
            LOG(FATAL) << "ERROR: WebApp not initialized. Use WebApp::init(argc, argv) before creating a concrete WebApp object";
            exit(1);
        }
    }

    WebApp::WebApp(const Router& router)
        : Router(router) {
    }

    void WebApp::init(int argc, char* argv[]) {
        net::EventLoop::init(argc, argv);
        WebApp::initialized = true;
    }

    void WebApp::start() {
        net::EventLoop::start();
    }

    void WebApp::stop() {
        net::EventLoop::stop();
    }

} // namespace express
