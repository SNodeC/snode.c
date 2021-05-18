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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "config.h" // just for this example app
#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/socket/bluetooth/rfcomm/legacy/SocketServer.h"

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::legacy;

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    SocketServer btServer(
        nullptr, // SharedFactory
        [](const SocketServer::SocketAddress& localAddress,
           const SocketServer::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + localAddress.toString();
            VLOG(0) << "\tClient: " + remoteAddress.toString();
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";
        },
        [](SocketServer::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
        },
        [](SocketServer::SocketConnection* socketConnection, const char* junk, std::size_t junkLen) -> void { // onRead
            std::string data(junk, junkLen);
            VLOG(0) << "Data to reflect: " << data;
            socketConnection->enqueue(data);
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            PLOG(ERROR) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            PLOG(ERROR) << "OnWriteError: " << errnum;
        });

    btServer.listen(SocketServer::SocketAddress("A4:B1:C1:2C:82:37", 1), 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            PLOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on channel 1";
        }
    });

    return net::SNodeC::start();
}
