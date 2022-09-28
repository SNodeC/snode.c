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

#include "broker/SocketContextFactory.h"

#include "broker/Broker.h"
#include "broker/SocketContext.h"
#include "core/socket/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    SocketContextFactory::SocketContextFactory()
        : broker(std::make_shared<mqtt::broker::Broker>()) {
    }

    core::socket::SocketContext* SocketContextFactory::create(core::socket::SocketConnection* socketConnection) {
        return new mqtt::broker::SocketContext(socketConnection, broker);
    }

} // namespace mqtt::broker
