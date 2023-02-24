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

#include "EchoSocketContext.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::echo::model {

    EchoSocketContext::EchoSocketContext(core::socket::stream::SocketConnection* socketConnection, Role role)
        : core::socket::stream::SocketContext(socketConnection)
        , role(role) {
    }

    void EchoSocketContext::onConnected() {
        VLOG(0) << "Echo connected";

        if (role == Role::CLIENT) {
            sendToPeer("Hello peer! Nice to see you!!!");
        }
    }

    void EchoSocketContext::onDisconnected() {
        VLOG(0) << "Echo disconnected";
    }

    std::size_t EchoSocketContext::onReceivedFromPeer() {
        char junk[4096];

        std::size_t junklen = readFromPeer(junk, 4096);

        if (junklen > 0) {
            VLOG(0) << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }

        return junklen;
    }

    core::socket::stream::SocketContext* EchoServerSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoSocketContext(socketConnection, EchoSocketContext::Role::SERVER);
    }

    core::socket::stream::SocketContext* EchoClientSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoSocketContext(socketConnection, EchoSocketContext::Role::CLIENT);
    }

} // namespace apps::echo::model
