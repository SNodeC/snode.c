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

#if (NETI != L2) // l2

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define SOCKETSERVER_INCLUDE QUOTE_INCLUDE(net/NET/stream/TYPE/SocketServer.h)
// clang-format on

#include SOCKETSERVER_INCLUDE

#include "apps/model/EchoSocketContext.h" // IWYU pragma: keep
#include "apps/model/lowlevelservers.h"
#include "core/SNodeC.h" // for SNodeC

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = net::NET::stream::TYPE::SocketServer<apps::model::EchoSocketContextFactory>; // this makes it an rf-EchoServer

#if (TYPEI == LEGACY) // legacy
    SocketServer server = apps::model::legacy::getServer<SocketServer>();
#elif (TYPEI == TLS) // tls
    SocketServer server = apps::model::tls::getServer<SocketServer>();
#endif

#if (NETI == IN) // in
    server.listen(8080, 5, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });
#elif (NETI == IN6) // in6
    server.listen(8080, 5, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });
#elif (NETI == L2)  // l2
    server.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            LOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on psm 0x1023";
        }
    });
#elif (NETI == RF)  // rf
    server.listen("A4:B1:C1:2C:82:37", 1, 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            PLOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on channel 1";
        }
    });
#elif (NETI == UN)  // un
    server.listen("/tmp/testme", 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            PLOG(ERROR) << "UN listen: " << errnum;
        } else {
            LOG(INFO) << "UN listening on /tmp/testme";
        }
    });
#endif
    return core::SNodeC::start();
}

#else

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    VLOG(0) << "Not implemented";
    return 0;
}

#endif
