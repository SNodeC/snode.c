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

#include "EchoSocketContext.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>     // for ssize_t
#include <string>      // for string
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::echo::model {

    EchoServerSocketContext::EchoServerSocketContext(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection, Role::SERVER) {
    }

    void EchoServerSocketContext::onConnected() {
        VLOG(0) << "Echo connected";
    }
    void EchoServerSocketContext::onDisconnected() {
        VLOG(0) << "Echo disconnected";
    }

    void EchoServerSocketContext::onReceiveFromPeer() {
        char junk[4096];

        ssize_t ret = readFromPeer(junk, 4096);

        if (ret > 0) {
            std::size_t junklen = static_cast<std::size_t>(ret);
            VLOG(0) << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }
    }

    void EchoServerSocketContext::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void EchoServerSocketContext::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    core::socket::stream::SocketContext* EchoServerSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoServerSocketContext(socketConnection);
    }

    EchoClientSocketContext::EchoClientSocketContext(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection, Role::CLIENT) {
    }

    void EchoClientSocketContext::onConnected() {
        VLOG(0) << "Echo connected";
        sendToPeer("Hello peer! Nice to see you!!!");
    }
    void EchoClientSocketContext::onDisconnected() {
        VLOG(0) << "Echo disconnected";
    }

    void EchoClientSocketContext::onReceiveFromPeer() {
        char junk[4096];

        ssize_t ret = readFromPeer(junk, 4096);

        if (ret > 0) {
            std::size_t junklen = static_cast<std::size_t>(ret);
            VLOG(0) << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }
    }

    void EchoClientSocketContext::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void EchoClientSocketContext::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    core::socket::stream::SocketContext* EchoClientSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoClientSocketContext(socketConnection);
    }

} // namespace apps::echo::model
