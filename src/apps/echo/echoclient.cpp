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

#define QUOTE_INCLUDE(a) STR(a)
#define STR(a) #a

// clang-format off
#define SOCKETCLIENT_INCLUDE QUOTE_INCLUDE(net/NET/stream/STREAM/SocketClient.h)
// clang-format on

#include SOCKETCLIENT_INCLUDE

#include "core/SNodeC.h"             // for SNodeC
#include "model/EchoSocketContext.h" // IWYU pragma: keep
#include "model/clients.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketClient =
        net::NET::stream::STREAM::SocketClient<apps::echo::model::EchoClientSocketContextFactory>; // this makes it an rf-EchoClient

    SocketClient client = apps::echo::model::STREAM::getClient<SocketClient>();

#if (NETI == IN) // in
    client.connect("localhost", 8080, [](int errnum) -> void {
#elif (NETI == IN6) // in6
    client.connect("localhost", 8080, [](int errnum) -> void {
#elif (NETI == L2)  // l2
    client.connect("A4:B1:C1:2C:82:37", 0x1023, "44:01:BB:A3:63:32", [](int errnum) -> void {
#elif (NETI == RF)  // rf
    client.connect("A4:B1:C1:2C:82:37", 1, "44:01:BB:A3:63:32", [](int errnum) -> void {
#elif (NETI == UN)  // un
    client.connect("/tmp/testme", [](int errnum) -> void {
#endif
        if (errnum != 0) {
            PLOG(ERROR) << "OnError: " << errnum;
        } else {
            VLOG(0) << "snode.c connected";
        }
    });

    return core::SNodeC::start();
}
