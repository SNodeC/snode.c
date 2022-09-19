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

#include "mqtt/ControlPacket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    ControlPacket::ControlPacket(SocketContext* socketContext, uint8_t type, uint8_t reserved, std::vector<char>& data)
        : socketContext(socketContext)
        , _type(type)
        , _reserved(reserved)
        , data(std::move(data)) {
    }

    uint8_t ControlPacket::type() {
        return _type;
    }

    uint8_t ControlPacket::reserved() {
        return _reserved;
    }

} // namespace mqtt
