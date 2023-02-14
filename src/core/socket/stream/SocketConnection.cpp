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

#include "core/socket/stream/SocketConnection.h"

#include "core/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace core::socket::stream {

    core::socket::stream::SocketContext*
    core::socket::stream::SocketConnection::switchSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        newSocketContext = socketContextFactory->create(this);

        if (newSocketContext == nullptr) {
            VLOG(0) << "Switch socket context unsuccessull: new socket context not created";
        }

        return newSocketContext;
    }

    bool SocketConnection::isValid() {
        return socketContext != nullptr;
    }

    SocketContext* SocketConnection::setSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory) {
        socketContext = socketContextFactory->create(this);

        if (socketContext == nullptr) {
            VLOG(0) << "Set socket context unsuccessull: new socket context not created";
        }

        return socketContext;
    }

} // namespace core::socket::stream