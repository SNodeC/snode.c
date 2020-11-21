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
#include "socket/bluetooth/l2cap/Socket.h"
#include "socket/sock_stream/legacy/SocketClient.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::stream;
using namespace net::socket::bluetooth;
using namespace net::socket::bluetooth::address;

legacy::SocketClient<net::socket::bluetooth::l2cap::Socket> getBtClient() {
    legacy::SocketClient<l2cap::Socket> btClient(
        []([[maybe_unused]] legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection) -> void { // onConstruct
        },
        []([[maybe_unused]] legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection) -> void { // onDestruct
        },
        [](legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("Hello l2cap connection!");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().address() + "(" +
                           socketConnection->getRemoteAddress().address() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().psm());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().address() + "(" + socketConnection->getLocalAddress().address() +
                           "):" + std::to_string(socketConnection->getLocalAddress().psm());
        },
        [](legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().address() + "(" +
                           socketConnection->getRemoteAddress().address() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().psm());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().address() + "(" + socketConnection->getLocalAddress().address() +
                           "):" + std::to_string(socketConnection->getLocalAddress().psm());
        },
        [](legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            std::string data(junk, junkSize);
            VLOG(0) << "Data to reflect: " << data;
            socketConnection->enqueue(data);
        },
        []([[maybe_unused]] legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] legacy::SocketClient<l2cap::Socket>::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " << errnum;
        },
        {{}});

    return btClient;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    {
        L2CapAddress remoteAddress("5C:C5:D4:B8:3C:AA", 0x1021); // calisto
        L2CapAddress bindAddress("44:01:BB:A3:63:32");           // mpow

        legacy::SocketClient legacyClient = getBtClient();

        legacyClient.connect(remoteAddress, bindAddress, [](int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });
    }

    return net::EventLoop::start();
}
