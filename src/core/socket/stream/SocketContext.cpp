/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/SocketContext.h"

#include "core/socket/stream/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    SocketContext::SocketContext(SocketConnection* socketConnection)
        : socketConnection(socketConnection) {
    }

    SocketContext* SocketContext::switchSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        return socketConnection->switchSocketContext(socketContextFactory);
    }

    SocketConnection* SocketContext::getSocketConnection() const {
        return socketConnection;
    }

    void SocketContext::sendToPeer(const char* junk, std::size_t junkLen) const {
        socketConnection->sendToPeer(junk, junkLen);
    }

    std::size_t SocketContext::readFromPeer(char* junk, std::size_t junklen) const {
        return socketConnection->readFromPeer(junk, junklen);
    }

    void SocketContext::setTimeout(const utils::Timeval& timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::close() {
        socketConnection->close();
    }

    void SocketContext::onConnected() {
        LOG(INFO) << "Protocol connected";
    }

    void SocketContext::onDisconnected() {
        LOG(INFO) << "Protocol disconnected";
    }

    void SocketContext::shutdownRead() {
        socketConnection->shutdownRead();
    }

    void SocketContext::shutdownWrite(bool forceClose) {
        socketConnection->shutdownWrite(forceClose);
    }

    void SocketContext::shutdown(bool forceClose) {
        shutdownRead();
        shutdownWrite(forceClose);
    }

    void SocketContext::onWriteError(int errnum) {
        Super::onWriteError(errnum);
        shutdownRead();
    }

    void SocketContext::onReadError(int errnum) { // By default we do a cross-shutdown. Override this method in case your protocol still
                                                  // wants to send data after peers sending side has closed the connection.

        Super::onReadError(errnum);
        shutdownWrite();
    }

} // namespace core::socket::stream
