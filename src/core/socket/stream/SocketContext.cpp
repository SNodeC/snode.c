/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    SocketContext::SocketContext(SocketConnection* socketConnection)
        : socketConnection(socketConnection) {
    }

    void SocketContext::switchSocketContext(SocketContext* newSocketContext) {
        socketConnection->switchSocketContext(newSocketContext);
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

    void SocketContext::shutdownRead() {
        socketConnection->shutdownRead();
    }

    void SocketContext::shutdownWrite(bool forceClose) {
        socketConnection->shutdownWrite(forceClose);
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        shutdownRead();
    }

    void SocketContext::onReadError([[maybe_unused]] int errnum) {
        shutdownWrite();
    }

} // namespace core::socket::stream
