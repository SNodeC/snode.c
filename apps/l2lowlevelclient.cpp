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

#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/socket/bluetooth/address/L2CapAddress.h" // for L2CapAddress
#include "net/socket/bluetooth/l2cap/Socket.h"         // for l2cap
#include "net/socket/bluetooth/l2cap/SocketClient.h"
#include "net/socket/stream/SocketClient.h"         // for SocketClient<...
#include "net/socket/stream/SocketConnectionBase.h" // for SocketConnect...
#include "net/socket/stream/SocketProtocol.h"
#include "net/socket/stream/SocketProtocolFactory.h"

#include <any> // for any
#include <cstddef>
#include <functional> // for function
#include <string>     // for string, alloc...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::l2cap;

class SimpleSocketProtocol : public net::socket::stream::SocketProtocol {
public:
    void receiveData(const char* junk, std::size_t junkLen) override {
        VLOG(0) << "Data to reflect: " << std::string(junk, junkLen);
        socketConnection->enqueue(junk, junkLen);
    }

    void onWriteError([[maybe_unused]] int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void onReadError([[maybe_unused]] int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
    }
};

class SimpleSocketProtocolFactory : public net::socket::stream::SocketProtocolFactory {
public:
    net::socket::stream::SocketProtocol* create() const override {
        return new SimpleSocketProtocol();
    }
};

SocketClient getClient() {
    SocketClient client(
        new SimpleSocketProtocolFactory(), // SharedFactory
        [](const SocketClient::SocketAddress& localAddress,
           const SocketClient::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        [](SocketClient::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            socketConnection->enqueue("Hello rfcomm connection!");
        },
        [](SocketClient::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        },
        {{}});

    return client;
}

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    {
        SocketClient::SocketAddress remoteAddress("A4:B1:C1:2C:82:37", 0x1023); // titan
        SocketClient::SocketAddress bindAddress("44:01:BB:A3:63:32");           // mpow

        SocketClient client = getClient();

        client.connect(remoteAddress, bindAddress, [](int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });
    }

    return net::SNodeC::start();
}
