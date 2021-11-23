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

#include "config.h"     // IWYU pragma: keep
#include "log/Logger.h" // for Writer

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define SOCKETSERVER_INCLUDE QUOTE_INCLUDE(net/NET/stream/STREAM/SocketServer.h)
// clang-format on

#include SOCKETSERVER_INCLUDE

#include "core/SNodeC.h"             // for SNodeC
#include "model/EchoSocketContext.h" // IWYU pragma: keep
#include "model/servers.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

#if (STREAM_TYPE == LEGACY)
    std::map<std::string, std::any> options = {{}};
#elif (STREAM_TYPE == TLS)
    std::map<std::string, std::any> options = {
        {"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}, {"caFile", CLIENTCAFILE}};

    std::map<std::string, std::any> sniOptions = {
        {"certChain", SNODECCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}, {"caFile", CLIENTCAFILE}};
#endif

    using SocketServer =
        net::NET::stream::STREAM::SocketServer<apps::echo::model::EchoServerSocketContextFactory>; // this makes it an rf-EchoServer

    SocketServer server = apps::echo::model::STREAM::getServer<SocketServer>(options);

#if (STREAM_TYPE == TLS)
    server.addSniCert("snodec.home.vchrist.at", sniOptions);
#endif

#if (NET_TYPE == IN) // in
    server.listen(8080, 5, [](int errnum) -> void {
#elif (NET_TYPE == IN6) // in6
    server.listen(8080, 5, [](int errnum) -> void {
#elif (NET_TYPE == L2)  // l2
    server.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](int errnum) -> void { // titan
#elif (NET_TYPE == RF)  // rf
    server.listen("A4:B1:C1:2C:82:37", 1, 5, [](int errnum) -> void { // titan
#elif (NET_TYPE == UN)  // un
    server.listen("/tmp/testme", 5, [](int errnum) -> void { // titan
#endif
        if (errnum != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening";
        }
    });

    return core::SNodeC::start();
}
