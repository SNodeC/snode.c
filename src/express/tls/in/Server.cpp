/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "express/tls/in/Server.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::tls::in {

    WebApp Server(const std::string& instanceName,
                  const express::Router& router,
                  const std::function<void(const std::string&, WebApp::SocketAddress, const core::socket::State&)>& reportState,
                  const std::function<void(typename WebApp::Config&)>& configurator) {
        using SocketAddress = typename WebApp::SocketAddress;

        const WebApp webApp(instanceName, router);

        if (configurator != nullptr) {
            configurator(webApp.getConfig());
        }

        webApp.listen([instanceName, reportState](const SocketAddress& socketAddress, const core::socket::State& state) {
            if (reportState != nullptr) {
                reportState(instanceName, socketAddress, state);
            } else {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            }
        });

        VLOG(1) << "Instance: " << instanceName;
        for (std::string& route : webApp.getRoutes()) {
            route.erase(std::remove(route.begin(), route.end(), '$'), route.end());

            VLOG(1) << "  " << route;
        }

        return webApp;
    }

    WebApp
    Server(const std::string& instanceName, const express::Router& router, const std::function<void(WebApp::Config&)>& configurator) {
        return Server(instanceName, router, nullptr, configurator);
    }

    WebApp Server(const std::string& instanceName,
                  const std::function<void(const std::string&, WebApp::SocketAddress, const core::socket::State&)>& reportState,
                  const std::function<void(typename WebApp::Config&)>& configurator) {
        return Server(instanceName, Router(), reportState, configurator);
    }

    WebApp Server(const std::string& instanceName, const std::function<void(WebApp::Config&)>& configurator) {
        return Server(instanceName, express::Router(), configurator);
    }

} // namespace express::tls::in
