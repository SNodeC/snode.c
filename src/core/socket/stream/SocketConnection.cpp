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

#include "core/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketConnection::~SocketConnection() {
    }

    core::socket::stream::SocketContext*
    SocketConnection::switchSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        newSocketContext = socketContextFactory->create(this);

        if (newSocketContext == nullptr) {
            VLOG(0) << "Switch socket context unsuccessull: new socket context not created";
        }

        return newSocketContext;
    }

    core::socket::stream::SocketContext*
    SocketConnection::setSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        socketContext = socketContextFactory->create(this);

        if (socketContext == nullptr) {
            VLOG(0) << "Set socket context unsuccessull: new socket context not created";
        }

        return socketContext;
    }

    bool SocketConnection::isValid() {
        return socketContext != nullptr;
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

} // namespace core::socket::stream
