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

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export

#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketConnection::SocketConnection(const std::string& instanceName)
        : instanceName(instanceName) {
    }

    SocketConnection::~SocketConnection() {
    }

    core::socket::stream::SocketContext*
    SocketConnection::switchSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        newSocketContext = socketContextFactory->create(this);

        if (newSocketContext == nullptr) {
            LOG(TRACE) << "SocketConnection: Socket context switch unsuccessull: socket context not created";
        }

        return newSocketContext;
    }

    core::socket::stream::SocketContext*
    SocketConnection::setSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        socketContext = socketContextFactory->create(this);

        if (socketContext == nullptr) {
            LOG(TRACE) << "SocketConnection: Set socket context unsuccessull: new socket context not created";
        }

        return socketContext;
    }

    void SocketConnection::sendToPeer(const std::string& data) {
        sendToPeer(data.data(), data.size());
    }

    void SocketConnection::sentToPeer(const std::vector<uint8_t>& data) {
        sendToPeer(reinterpret_cast<const char*>(data.data()), data.size());
    }

    void SocketConnection::sentToPeer(const std::vector<char>& data) {
        sendToPeer(data.data(), data.size());
    }

    const std::string& SocketConnection::getInstanceName() const {
        return instanceName;
    }

    void SocketConnection::connected(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory) {
        if (setSocketContext(socketContextFactory.get()) != nullptr) {
            socketContext->onConnected();
        } else {
            LOG(TRACE) << "SocketConnection: Failed creating new SocketContext";
            close();
        }
    }

    void SocketConnection::connected(core::socket::stream::SocketContext* socketContext) {
        socketContext->onConnected();
    }

    void SocketConnection::disconnected() {
        if (socketContext != nullptr) {
            socketContext->onDisconnected();
            delete socketContext;
        }
    }

} // namespace core::socket::stream
