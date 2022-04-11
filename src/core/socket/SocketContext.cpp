/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/socket/SocketContext.h"

#include "core/socket/SocketConnection.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
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

    SocketContext* SocketContext::switchSocketContext(core::socket::SocketContextFactory* socketContextFactory) {
        return socketConnection->switchSocketContext(socketContextFactory);
    }

    void SocketContext::setTimeout(const utils::Timeval& timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::onConnected() {
        PLOG(INFO) << "Protocol connected";
    }

    void SocketContext::onDisconnected() {
        PLOG(INFO) << "Protocol disconnected";
    }

    void SocketContext::shutdownRead() {
        socketConnection->shutdownRead();
    }

    void SocketContext::shutdownWrite() {
        socketConnection->shutdownWrite();
    }

    void SocketContext::shutdown() {
        shutdownRead();
        shutdownWrite();
    }

    void SocketContext::close() {
        socketConnection->close();
    }

    void SocketContext::onWriteError(int errnum) {
        if (errnum != 0) {
            PLOG(ERROR) << "OnWriteError: " << errnum;
        }
        shutdownRead();
    }

    void SocketContext::onReadError(int errnum) { // By default we do a cross-shutdown. Override this method in case your protocol still
                                                  // wants to send data after peers sending side has closed the connection.
        if (errnum != 0) {
            PLOG(ERROR) << "OnReadError: " << errnum;
        }
        shutdownWrite();
    }

    void SocketContext::receive(const char* junk, std::size_t junkLen) {
        sendToPeer(junk, junkLen);
    }

    void SocketContext::eof() {
        PLOG(INFO) << "Stream EOF";
    }

    void SocketContext::error([[maybe_unused]] int errnum) {
        PLOG(ERROR) << "Stream error:";
        close();
    }

} // namespace core::socket
