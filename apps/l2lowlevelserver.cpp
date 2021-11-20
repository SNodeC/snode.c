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

#include "log/Logger.h"                              // for Writer, Storage
#include "core/SNodeC.h"                              // for SNodeC
#include "core/socket/bluetooth/l2cap/SocketServer.h" // for SocketServer
#include "core/socket/stream/SocketContext.h"         // for SocketProtocol
#include "core/socket/stream/SocketContextFactory.h"  // for SocketProtoco...

#include <cstddef>     // for size_t
#include <functional>  // for function
#include <string>      // for allocator
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace core::socket::bluetooth::l2cap;

class SimpleSocketProtocol : public core::socket::stream::SocketContext {
public:
    explicit SimpleSocketProtocol(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection) {
    }

    void onReceiveFromPeer() override {
        char junk[4096];

        ssize_t ret = readFromPeer(junk, 4096);

        if (ret > 0) {
            std::size_t junklen = static_cast<std::size_t>(ret);
            VLOG(0) << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }
    }

    void onWriteError(int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void onReadError(int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
    }
};

class SimpleSocketProtocolFactory : public core::socket::stream::SocketContextFactory {
public:
    ~SimpleSocketProtocolFactory() = default;

    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new SimpleSocketProtocol(socketConnection);
    }
};

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    SocketServer<SimpleSocketProtocolFactory> btServer(
        [](const SocketServer<SimpleSocketProtocolFactory>::SocketAddress& localAddress,
           const SocketServer<SimpleSocketProtocolFactory>::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: (" + localAddress.address() + ") " + localAddress.toString();
            VLOG(0) << "\tClient: (" + remoteAddress.address() + ") " + remoteAddress.toString();
        },
        []([[maybe_unused]] SocketServer<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

        },
        [](SocketServer<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

    btServer.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            LOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on psm 0x1023";
        }
    });

    return core::SNodeC::start();
}
