/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "EventLoop.h"
#include "Logger.h"
#include "config.h" // just for this example app
#include "socket/bluetooth/rfcomm/legacy/SocketServer.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::legacy;

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    SocketServer btServer(
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onConstruct
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onDestruct
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onConnect
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onDisconnect
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection,
           [[maybe_unused]] const char* junk,
           [[maybe_unused]] ssize_t junkLen) -> void { // onRead
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, [[maybe_unused]] int errnum) -> void { // onReadError
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, [[maybe_unused]] int errnum) -> void { // onWriteError
        });

    btServer.listen("5C:C5:D4:B8:3C:AA", 1, 5, [](int errnum) -> void {
        if (errnum != 0) {
            LOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on channel 1";
        }
    });

    return net::EventLoop::start();
}
