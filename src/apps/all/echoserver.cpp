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

#include "log/Logger.h" // for Writer

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define SOCKETSERVER_INCLUDE QUOTE_INCLUDE(net/NET/stream/STREAM/SocketServer.h)
// clang-format on

#include SOCKETSERVER_INCLUDE

#include "apps/all/model/EchoSocketContext.h" // IWYU pragma: keep
#include "apps/all/model/servers.h"
#include "core/SNodeC.h" // for SNodeC

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = net::NET::stream::STREAM::SocketServer<apps::all::model::EchoSocketContextFactory>; // this makes it an rf-EchoServer

    SocketServer server = apps::model::STREAM::getServer<SocketServer>();

#if (NETI == IN) // in
    server.listen(8080, 5, [](int errnum) -> void {
#elif (NETI == IN6) // in6
    server.listen(8080, 5, [](int errnum) -> void {
#elif (NETI == L2)  // l2
    server.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](int errnum) -> void { // titan
#elif (NETI == RF)  // rf
    server.listen("A4:B1:C1:2C:82:37", 1, 5, [](int errnum) -> void { // titan
#elif (NETI == UN)  // un
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
