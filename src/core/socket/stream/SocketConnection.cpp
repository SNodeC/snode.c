/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/socket/stream/SocketConnection.h"

#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketConnection::SocketConnection(const std::string& instanceName, int fd, const std::string& configuredServer)
        : instanceName(instanceName)
        , connectionName("[" + std::to_string(fd) + "]" + (!instanceName.empty() ? " " : "") + instanceName)
        , configuredServer(configuredServer) {
    }

    SocketConnection::~SocketConnection() {
    }

    void SocketConnection::switchSocketContext(SocketContext* newSocketContext) {
        this->newSocketContext = newSocketContext;
    }

    void SocketConnection::setSocketContext(SocketContext* socketContext) {
        if (socketContext != nullptr) { // Perform a pending SocketContextSwitch
            LOG(DEBUG) << connectionName << ": Connecting SocketContext";
            this->socketContext = socketContext;
            socketContext->onConnected();
        } else {
            LOG(ERROR) << connectionName << ": Connecting SocketContext failed: no new SocketContext";
        }
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

    const std::string& SocketConnection::getConnectionName() const {
        return connectionName;
    }

    const std::string& SocketConnection::getConfiguredServer() const {
        return configuredServer;
    }

    void SocketConnection::connectSocketContext(const std::shared_ptr<SocketContextFactory>& socketContextFactory) {
        SocketContext* socketContext = socketContextFactory->create(this);

        if (socketContext != nullptr) {
            LOG(DEBUG) << connectionName << ": Creating SocketContext successful";
            setSocketContext(socketContext);
        } else {
            LOG(ERROR) << connectionName << ": Failed creating SocketContext";
            close();
        }
    }

    void SocketConnection::disconnectCurrentSocketContext() {
        if (socketContext != nullptr) {
            socketContext->onDisconnected();
            delete socketContext;

            LOG(DEBUG) << connectionName << ": SocketContext disconnected";
        }
    }

} // namespace core::socket::stream
