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

#include "core/socket/SocketConnection.h"

#include "core/socket/SocketContext.h"
#include "core/socket/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    SocketConnection::SocketConnection(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory)
        : socketContext(socketContextFactory->create(this)) {
    }

    SocketConnection::~SocketConnection() {
        delete socketContext;
    }

    void SocketConnection::onConnected() {
        socketContext->onConnected();
    }

    void SocketConnection::onDisconnected() {
        socketContext->onDisconnected();
    }

    void SocketConnection::onReceiveFromPeer() {
        socketContext->onReceiveFromPeer();

        if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
            onDisconnected();
            delete socketContext;
            socketContext = newSocketContext;
            newSocketContext = nullptr;
            onConnected();
        }
    }

    void SocketConnection::onWriteError(int errnum) {
        socketContext->onWriteError(errnum);
    }

    void SocketConnection::onReadError(int errnum) {
        socketContext->onReadError(errnum);
    }

    core::socket::SocketContext* SocketConnection::getSocketContext() {
        return socketContext;
    }

    core::socket::SocketContext* SocketConnection::switchSocketContext(core::socket::SocketContextFactory* socketContextFactory) {
        newSocketContext = socketContextFactory->create(this);

        if (newSocketContext == nullptr) {
            VLOG(0) << "Switch socket context unsuccessull: new socket context not created";
        }

        return newSocketContext;
    }

} // namespace core::socket
