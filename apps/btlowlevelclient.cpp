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

#include "log/Logger.h"                                      // for Writer
#include "net/SNodeC.h"                                      // for SNodeC
#include "net/socket/bluetooth/address/RfCommAddress.h"      // for RfCommA...
#include "net/socket/bluetooth/rfcomm/legacy/SocketClient.h" // for SocketC...
#include "net/socket/stream/SocketClient.h"                  // for SocketC...
#include "net/socket/stream/SocketContext.h"                // for SocketP...
#include "net/socket/stream/SocketContextFactory.h"         // for SocketP...

#include <any>        // for any
#include <cstddef>    // for size_t
#include <functional> // for function
#include <string>     // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::legacy;

class SimpleSocketProtocol : public net::socket::stream::SocketContext {
public:
    void onReceiveFromPeer(const char* junk, std::size_t junkLen) override {
        VLOG(0) << "Data to reflect: " << std::string(junk, junkLen);
        sendToPeer(junk, junkLen);
    }

    void onWriteError(int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void onReadError(int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
    }
};

class SimpleSocketProtocolFactory : public net::socket::stream::SocketContextFactory {
private:
    net::socket::stream::SocketContext* create() const override {
        return new SimpleSocketProtocol();
    }
};

SocketClient<SimpleSocketProtocolFactory> getClient() {
    return SocketClient<SimpleSocketProtocolFactory>(
        [](const SocketClient<SimpleSocketProtocolFactory>::SocketAddress& localAddress,
           const SocketClient<SimpleSocketProtocolFactory>::SocketAddress& remoteAddress) -> void { // onConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        [](SocketClient<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            socketConnection->enqueue("Hello rfcomm connection!");
        },
        [](SocketClient<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        },
        {{}});
}

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    SocketClient<SimpleSocketProtocolFactory>::SocketAddress remoteAddress("A4:B1:C1:2C:82:37", 1); // titan
    SocketClient<SimpleSocketProtocolFactory>::SocketAddress bindAddress("44:01:BB:A3:63:32");      // mpow

    SocketClient client = getClient();

    client.connect(remoteAddress, bindAddress, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return net::SNodeC::start();
}
