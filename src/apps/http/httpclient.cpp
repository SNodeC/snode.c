/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "apps/http/model/clients.h"
#include "core/SNodeC.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Client = apps::http::STREAM::Client;

    const Client client = apps::http::STREAM::getClient();

    client.connect([instanceName = client.getConfig().getInstanceName()](
                       const core::socket::SocketAddress& socketAddress,
                       const core::socket::State& state) { // example.com:81 simulate connnect timeout
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
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

    return core::SNodeC::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    client.connect("localhost", 8080, [](const SocketAddress& socketAddress, const core::socket::State& state) {
#elif (STREAM_TYPE == TLS)
    client.connect("localhost", 8088, [](const SocketAddress& socketAddress, const core::socket::State& state) {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
        client.connect("localhost", 8080, [](const SocketAddress& socketAddress, const core::socket::State& state) {
#elif (STREAM_TYPE == TLS)
        client.connect("localhost", 8088, [](const SocketAddress& socketAddress, const core::socket::State& state) {
#endif
#elif (NET_TYPE == L2) // l2
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // client.connect("A4:B1:C1:2C:82:37", 0x1023, "44:01:BB:A3:63:32", [](const SocketAddress& socketAddress, const core::socket::State&
state) { client.connect("10:3D:1C:AC:BA:9C", 0x1023, "44:01:BB:A3:63:32", [](const SocketAddress& socketAddress, const
core::socket::State& state) { #elif (NET_TYPE == RC) // rf
    // client.connect("A4:B1:C1:2C:82:37", 1, "44:01:BB:A3:63:32", [](const SocketAddress& socketAddress, const core::socket::State& state)
{ client.connect("10:3D:1C:AC:BA:9C", 1, "44:01:BB:A3:63:32", [](const SocketAddress& socketAddress, const core::socket::State&
state) { #elif (NET_TYPE == UN) // un client.connect("/tmp/testme", [](const SocketAddress& socketAddress, const
core::socket::State& state) { #endif if (errnum != 0) { PLOG(ERROR) << "OnError: " << errnum; } else { VLOG(1) << "snode.c
connecting to " << socketAddress.toString();
        }

#ifdef NET_TYPE
    });
#endif
*/
