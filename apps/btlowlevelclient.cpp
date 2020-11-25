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
#include "ResponseParser.h"
#include "ServerResponse.h"
#include "config.h" // just for this example app
#include "socket/bluetooth/rfcomm/legacy/SocketClient.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::legacy;

SocketClient getClient() {
    SocketClient client(
        []([[maybe_unused]] SocketClient::SocketConnection* socketConnection) -> void { // onConstruct
        },
        []([[maybe_unused]] SocketClient::SocketConnection* socketConnection) -> void { // onDestruct
        },
        [](SocketClient::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

            socketConnection->enqueue("Hello rfcomm connection!");
        },
        [](SocketClient::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        },
        [](SocketClient::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            std::string data(junk, junkSize);
            VLOG(0) << "Data to reflect: " << data;
            socketConnection->enqueue(data);
        },
        []([[maybe_unused]] SocketClient::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            PLOG(ERROR) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] SocketClient::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            PLOG(ERROR) << "OnWriteError: " << errnum;
        },
        {{}});

    return client;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    SocketClient::SocketAddress remoteAddress("5C:C5:D4:B8:3C:AA", 1); // calisto
    SocketClient::SocketAddress bindAddress("44:01:BB:A3:63:32");      // mpow

    SocketClient client = getClient();

    client.connect(remoteAddress, bindAddress, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return net::EventLoop::start();
}
