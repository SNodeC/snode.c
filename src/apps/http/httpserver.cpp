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

#include "apps/http/model/servers.h"
#include "config.h" // just for this example app
#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) { // cppcheck-suppress syntaxError
    logger::Logger::setVerboseLevel(2);

    WebApp::init(argc, argv);

#if (STREAM_TYPE == LEGACY)
    std::map<std::string, std::any> options{};
#elif (STREAM_TYPE == TLS)
    std::map<std::string, std::any> options{{"CertChain", SERVERCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}};
    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
        {"snodec.home.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}}};
#endif

    using WebApp = apps::http::STREAM::WebApp;
    WebApp webApp(apps::http::STREAM::getWebApp("httpserver", SERVERROOT, options));

#if (STREAM_TYPE == TLS)
    webApp.addSniCerts(sniCerts);
#endif

    webApp.listen([](const WebApp::Socket& socket, int errnum) -> void {
        if (errnum != 0) {
            PLOG(FATAL) << "listen";
        } else {
            VLOG(0) << "snode.c listening on " << socket.getBindAddress().toString();
        }
    }); // cppcheck-suppress syntaxError

    /*
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
    #elif (NET_TYPE == L2) //
            // ATLAS: 10:3D:1C:AC:BA:9C
            // TITAN: A4:B1:C1:2C:82:37
            // USB: 44:01:BB:A3:63:32

        // webApp.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
             webApp.listen("10:3D:1C:AC:BA:9C", 0x1023, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
    #elif (NET_TYPE == RF) // rf
            // webApp.listen("A4:B1:C1:2C:82:37", 1, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
             webApp.listen("10:3D:1C:AC:BA:9C", 1, 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
    #elif (NET_TYPE == UN) // un
             webApp.listen("/tmp/testme", 5, [](const WebApp::Socket& socket, int errnum) -> void { // titan
    #endif
            if (errnum != 0) {
                PLOG(FATAL) << "listen";
            } else {
                VLOG(0) << "snode.c listening on " << socket.getBindAddress().toString();
            }

    #ifdef NET_TYPE
        }); // cppcheck-suppress syntaxError
    #endif
    */

    return WebApp::start();
}
