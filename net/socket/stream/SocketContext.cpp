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
#include "net/socket/stream/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    SocketContext::SocketContext(SocketConnection* socketConnection)
        : socketConnection(socketConnection) {
    }

    void SocketContext::sendToPeer(const char* junk, std::size_t junkLen) {
        socketConnection->sendToPeer(junk, junkLen);
    }

    void SocketContext::sendToPeer(const std::string& data) {
        sendToPeer(data.data(), data.length());
    }

    ssize_t SocketContext::readFromPeer(char* junk, std::size_t junklen) {
        return socketConnection->readFromPeer(junk, junklen);
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

    void SocketContext::switchSocketContext(SocketContextFactory* socketContextFactory) {
        stop();
        socketConnection->switchSocketContext(socketContextFactory);
    }

    void SocketContext::receiveFromPeer() {
        onReceiveFromPeer();

        if (this != socketConnection->getSocketContext()) {
            socketConnection->getSocketContext()->onReceiveFromPeer(); // There can be more data already read ... process it!
            delete this;                                               // we were replaced by a different context.
        }
    }

    void SocketContext::setTimeout(int timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::onConnected() {
        VLOG(0) << "Protocol connected";
    }

    void SocketContext::onDisconnected() {
        VLOG(0) << "Protocol disconnecteded";
    }

} // namespace net::socket::stream
