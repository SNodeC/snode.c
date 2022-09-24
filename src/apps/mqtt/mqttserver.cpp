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

#include "apps/mqtt/server/SocketContextFactory.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "apps/mqtt/server/Broker.h"
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/legacy/SocketServer.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

using MQTTServer = net::in::stream::legacy::SocketServer<apps::mqtt::server::SocketContextFactory>;
using SocketConnection = MQTTServer::SocketConnection;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    core::SNodeC::init(argc, argv);

    MQTTServer mqttServer(
        "server",
        []([[maybe_unused]] SocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
        },
        []([[maybe_unused]] SocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";
        },
        []([[maybe_unused]] SocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";
        });

    mqttServer.listen([](const MQTTServer::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "mqttserver listening on " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}
