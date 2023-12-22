/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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
#include "log/Logger.h"
#include "utils/Config.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

int main(int argc, char* argv[]) {
    std::string webRoot;
    utils::Config::add_string_option("--web-root", webRoot, "Root directory of the web site", "[path]");

    WebApp::init(argc, argv);

    using WebApp = apps::http::STREAM::WebApp;
    using SocketAddress = WebApp::SocketAddress;

    WebApp webApp(apps::http::STREAM::getWebApp("httpserver", webRoot));

#if (STREAM_TYPE == TLS)
    //    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
    //        {"snodec.home.vchrist.at", {{"CertChain", certChainFile}, {"CertChainKey", keyFile}, {"Password", password}, {"CaFile",
    //        caFile}}},
    //        {"www.vchrist.at", {{"CertChain", certChainFile}, {"CertChainKey", keyFile}, {"Password", password}, {"CaFile", caFile}}}};

    //    webApp.addSniCerts(sniCerts);
    //    webApp.forceSni();
#endif
    webApp.listen([instanceName = webApp.getConfig().getInstanceName()](const SocketAddress& socketAddress,
                                                                        const core::socket::State& state) -> void {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << instanceName << ": disabled";
                break;
            case core::socket::State::ERROR:
                LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                break;
            case core::socket::State::FATAL:
                LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                break;
        }
    });

    return WebApp::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    webApp.listen(8080, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
#elif (STREAM_TYPE == TLS)
    webApp.listen(8088, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
        webApp.listen(8080, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
#elif (STREAM_TYPE == TLS)
        webApp.listen(8088, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
#endif
#elif (NET_TYPE == L2) //
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // webApp.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) ->
void { // titan webApp.listen("10:3D:1C:AC:BA:9C", 0x1023, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State&
state) -> void { // titan #elif (NET_TYPE == RC) // rf
    // webApp.listen("A4:B1:C1:2C:82:37", 1, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
// titan webApp.listen("10:3D:1C:AC:BA:9C", 1, 5, [](const WebApp::SocketAddress& socketAddress, const core::socket::State& state) -> void {
// titan #elif (NET_TYPE == UN) // un webApp.listen("/tmp/testme", 5, [](const WebApp::SocketAddress& socketAddress, const
core::socket::State& state)
-> void { // titan #endif if (errnum != 0) { PLOG(FATAL) << "listen"; } else { VLOG(0) << "snode.c listening on " <<
socketAddress.toString();
        }

#ifdef NET_TYPE
    });
#endif
*/
