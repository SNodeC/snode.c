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

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "model/clients.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketClient = apps::echo::model::STREAM::EchoSocketClient;
    using SocketAddress = SocketClient::SocketAddress;
    SocketClient client = apps::echo::model::STREAM::getClient();

    client.connect([](const SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            //            errno = errnum;
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c connecting to " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    client.connect("localhost", 8080, [](int errnum) -> void {
#elif (STREAM_TYPE == TLS)
    client.connect("localhost", 8088, [](int errnum) -> void {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
    client.connect("localhost", 8080, [](int errnum) -> void {
#elif (STREAM_TYPE == TLS)
    client.connect("localhost", 8088, [](int errnum) -> void {
#endif
#elif (NET_TYPE == L2) // l2
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // client.connect("A4:B1:C1:2C:82:37", 0x1023, "44:01:BB:A3:63:32", [](int errnum) -> void {
    client.connect("10:3D:1C:AC:BA:9C", 0x1023, "44:01:BB:A3:63:32", [](int errnum) -> void {
#elif (NET_TYPE == RC) // rf
    // client.connect("A4:B1:C1:2C:82:37", 1, "44:01:BB:A3:63:32", [](int errnum) -> void {
    client.connect("10:3D:1C:AC:BA:9C", 1, "44:01:BB:A3:63:32", [](int errnum) -> void {
#elif (NET_TYPE == UN) // un
    client.connect("/tmp/testme", [](int errnum) -> void {
#endif

    if (errnum < 0) {
        PLOG(ERROR) << "OnError";
    } else if (errnum > 0) {
        errno = errnum;
        PLOG(ERROR) << "OnError: " << socketAddress.toString();
    } else {
        VLOG(0) << "snode.c connecting to " << socketAddress.toString();
    }

#ifdef NET_TYPE
    });
#endif
*/
