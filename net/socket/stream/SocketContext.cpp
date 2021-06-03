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

#include "net/socket/stream/SocketContext.h"

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    void SocketContext::sendToPeer(const char* junk, std::size_t junkLen) {
        socketConnection->enqueue(junk, junkLen);
    }

    void SocketContext::sendToPeer(const std::string& data) {
        sendToPeer(data.data(), data.length());
    }

    std::string SocketContext::getLocalAddressAsString() const {
        return socketConnection->getLocalAddressAsString();
    }

    std::string SocketContext::getRemoteAddressAsString() const {
        return socketConnection->getRemoteAddressAsString();
    }

    void SocketContext::close() {
        socketConnection->close();
    }

    void SocketContext::switchSocketProtocol(SocketContext* socketProtocol) {
        socketConnection->switchSocketProtocol(socketProtocol);

        markedForDelete = true;
    }

    void SocketContext::receiveFromPeer(const char* junk, std::size_t junkLen) {
        onReceiveFromPeer(junk, junkLen);

        if (markedForDelete) {
            delete this;
        }
    }

    void SocketContext::setTimeout(int timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::setSocketConnection(SocketConnectionBase* socketConnection) {
        this->socketConnection = socketConnection;
    }

    void SocketContext::onProtocolConnected() {
        VLOG(0) << "Protocol connected";
    }

    void SocketContext::onProtocolDisconnected() {
        VLOG(0) << "Protocol disconnecteded";
    }

} // namespace net::socket::stream
