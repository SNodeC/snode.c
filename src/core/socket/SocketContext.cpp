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

#include "core/socket/SocketContext.h"

#include "core/socket/SocketConnection.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    SocketContext::SocketContext(SocketConnection* socketConnection, Role role)
        : socketConnection(socketConnection)
        , role(role) {
    }

    void SocketContext::sendToPeer(const char* junk, std::size_t junkLen) {
        socketConnection->sendToPeer(junk, junkLen);
    }

    void SocketContext::sendToPeer(const std::string& data) {
        sendToPeer(data.data(), data.length());
    }

    ssize_t SocketContext::readFromPeer(char* junk, std::size_t junklen) {
        ssize_t ret = 0;

        ret = socketConnection->readFromPeer(junk, junklen);

        return ret;
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

    SocketContext* SocketContext::switchSocketContext(SocketContextFactory* socketContextFactory) {
        return socketConnection->switchSocketContext(socketContextFactory);
    }

    void SocketContext::setTimeout(int timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::onConnected() {
        VLOG(0) << "Protocol connected";
    }

    void SocketContext::onDisconnected() {
        VLOG(0) << "Protocol disconnected";
    }

    SocketContext::Role SocketContext::getRole() const {
        return role;
    }

    void SocketContext::shutdownRead() {
        socketConnection->shutdownRead();
    }

    void SocketContext::shutdownWrite() {
        socketConnection->shutdownWrite();
    }

    void SocketContext::onWriteError([[maybe_unused]] int errnum) {
        PLOG(ERROR) << "OnWriteError: " << errnum;
    }

    void SocketContext::onReadError([[maybe_unused]] int errnum) {
        PLOG(ERROR) << "OnReadError: " << errnum;
        //        if (errnum == 0) { // if we get EOF gracefully shut down also the write side
        shutdownWrite();
        //        }
    }

} // namespace core::socket
