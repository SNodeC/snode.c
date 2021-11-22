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

#define QUOTE_INCLUDE(a) STR(a)
#define STR(a) #a

// clang-format off
#define SOCKETCLIENT_INCLUDE QUOTE_INCLUDE(net/NET/stream/TYPE/SocketClient.h)
// clang-format on

#include SOCKETCLIENT_INCLUDE

#include "apps/model/EchoSocketContext.h" // IWYU pragma: keep
#include "apps/model/lowlevelclients.h"
#include "core/SNodeC.h" // for SNodeC

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketClient = net::NET::stream::TYPE::SocketClient<apps::model::EchoSocketContextFactory>; // this makes it an rf-EchoClient

#if (TYPEI == LEGACY) // legacy
    SocketClient client = apps::model::legacy::getClient<SocketClient>();
#elif (TYPEI == TLS) // tls
    SocketClient client = apps::model::tls::getClient<SocketClient>();
#endif

#if (NETI == IN) // in
    client.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });
#elif (NETI == IN6) // in6
    client.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });
#elif (NETI == L2)  // l2
    client.connect("A4:B1:C1:2C:82:37", 0x1023, "44:01:BB:A3:63:32", [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << err;
        } else {
            VLOG(0) << "Connected";
        }
    });
#elif (NETI == RF)  // rf
    client.connect("A4:B1:C1:2C:82:37", 1, "44:01:BB:A3:63:32", [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << err;
        } else {
            VLOG(0) << "Connected";
        }
    });
#elif (NETI == UN)  // un
    client.connect("/tmp/testme", [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
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
