/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "ConfigWWW.h"
#include "apps/http/model/servers.h"
#include "express/middleware/StaticMiddleware.h"
#include "net/config/ConfigInstanceAPI.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#if (STREAM_TYPE == TLS) // tls

#include <utility>
#include <variant>

#endif // (STREAM_TYPE == TLS)

#include "log/Logger.h"

#include <algorithm>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    using WebApp = apps::http::STREAM::WebApp;

    const WebApp webApp(apps::http::STREAM::getWebApp("httpserver"));

    webApp.getConfig().addSection<section::ConfigWWW>();

    WebApp::init(argc, argv);

    webApp.use(express::middleware::StaticMiddleware(webApp.getConfig().getSection<section::ConfigWWW>()->getHtmlRoot()));

    {
#if (STREAM_TYPE == TLS)
        std::string cert = "/home/voc/projects/snodec/snode.c/certs/snode.c_-_server.pem";
        std::string key = "/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem";
        std::string pass = "snode.c";

        std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>> const sniCerts = {
            {"snodec.home.vchrist.at", {{"Cert", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}},
            {"www.vchrist.at", {{"Cert", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}}};

        // webApp.getConfig().addSniCerts(sniCerts);
#endif

        VLOG(1) << "Routes:";
        for (std::string& route : webApp.getRoutes()) {
            route.erase(std::remove(route.begin(), route.end(), '$'), route.end());

            VLOG(1) << "  " << route;
        }

        webApp.listen([instanceName = webApp.getConfig().getInstanceName()](const core::socket::SocketAddress& socketAddress,
                                                                            const core::socket::State& state) {
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
        });
    }
    return WebApp::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    webApp.listen(8080, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
#elif (STREAM_TYPE == TLS)
    webApp.listen(8088, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
    webApp.listen(8080, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
#elif (STREAM_TYPE == TLS)
    webApp.listen(8088, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
#endif
#elif (NET_TYPE == L2) //
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // webApp.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) ->
void { // titan webApp.listen("10:3D:1C:AC:BA:9C", 0x1023, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State&
state) { // titan #elif (NET_TYPE == RC) // rf
    // webApp.listen("A4:B1:C1:2C:82:37", 1, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
// titan webApp.listen("10:3D:1C:AC:BA:9C", 1, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
// titan #elif (NET_TYPE == UN) // un webApp.listen("/tmp/testme", 5, [](const WebApp::SocketAddress& socketAddress, const
core::socket::State& state) { // titan #endif if (errnum != 0) { PLOG(FATAL) << "listen"; } else { VLOG(1) << "snode.c listening on
" << socketAddress.toString();
        }

#ifdef NET_TYPE
    });
#endif
*/
