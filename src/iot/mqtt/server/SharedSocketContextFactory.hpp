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

#include "iot/mqtt/server/SharedSocketContextFactory.h" // IWYU pragma: export
#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    template <typename SocketContext>
    SharedSocketContextFactory<SocketContext>::SharedSocketContextFactory()
        : broker(iot::mqtt::server::broker::Broker::instance(SUBSCRIBTION_MAX_QOS)) {
    }

    template <typename SocketContext>
    core::socket::SocketContext* SharedSocketContextFactory<SocketContext>::create(core::socket::SocketConnection* socketConnection) {
        return create(socketConnection, broker);
    }

} // namespace iot::mqtt::server
