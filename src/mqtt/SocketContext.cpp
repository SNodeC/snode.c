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

#include "mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection)
        , controlPacketFactory(this) {
    }

    std::size_t SocketContext::onReceiveFromPeer() {
        VLOG(0) << "SocketContext: onReceiveFromPeer";

        std::size_t consumed = controlPacketFactory.construct();

        if (controlPacketFactory.isError()) {
            VLOG(0) << "SocketContext: Error during ControlPacket construction";
            close();
        } else if (controlPacketFactory.complete()) {
            onControlPackageReceived(controlPacketFactory.get());
        }

        return consumed;
    }

} // namespace mqtt
