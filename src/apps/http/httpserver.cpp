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

#include "apps/http/model/servers.h"
#include "config.h" // just for this example app
#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    logger::Logger::setVerboseLevel(2);

    WebApp::init(argc, argv);

#if (STREAM_TYPE == LEGACY)
    std::map<std::string, std::any> options{};
#elif (STREAM_TYPE == TLS)
    std::map<std::string, std::any> options{{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}};
#endif

    using WebApp = express::STREAM::NET::WebApp;
    WebApp webApp(apps::http::STREAM::getWebApp(options));

#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    webApp.listen(8080, 5, [](const WebApp::Socket& socket, int errnum) -> void {
#elif (STREAM_TYPE == TLS)
    webApp.listen(8088, 5, [](const WebApp::Socket& socket, int errnum) -> void {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
    webApp.listen(8080, 5, [](const WebApp::Socket& socket, int errnum) -> void {
#elif (STREAM_TYPE == TLS)
    webApp.listen(8088, 5, [](const WebApp::Socket& socket, int errnum) -> void {
#endif
#elif (NET_TYPE == L2) // l2
    webApp.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
#elif (NET_TYPE == RF) // rf
    webApp.listen("A4:B1:C1:2C:82:37", 1, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
#elif (NET_TYPE == UN) // un
    webApp.listen("/tmp/testme", 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
#endif
        if (errnum != 0) {
            PLOG(FATAL) << "listen";
        } else {
            VLOG(0) << "snode.c listening on " << socket.getBindAddress().toString();
        }
    });

    return WebApp::start();
}
