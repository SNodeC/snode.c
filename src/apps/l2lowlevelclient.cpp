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

#include "apps/model/EchoSocketContext.h" // IWYU pragma: keep
#include "apps/model/lowlevellegacyclient.h"
#include "core/SNodeC.h"                       // for SNodeC
#include "log/Logger.h"                        // for Writer
#include "net/l2/stream/legacy/SocketClient.h" // for SocketC...

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory> // for allocator

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    // "A4:B1:C1:2C:82:37" titan
    // "44:01:BB:A3:63:32"  mpow

    using SocketClient = net::l2::stream::legacy::SocketClient<apps::model::EchoSocketContextFactory>; // this makes it an rf-EchoClient

    SocketClient client = apps::model::lowlevellegacy::getClient<SocketClient>();

    client.connect("A4:B1:C1:2C:82:37", 0x1023, "44:01:BB:A3:63:32", [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << err;
        } else {
            VLOG(0) << "Connected";
        }
    });

    return core::SNodeC::start();
}
