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

#include "mqtt/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::packets {

    Connect::Connect(SocketContext* socketContext, uint8_t type, uint8_t reserved, std::vector<char>& data)
        : mqtt::ControlPacket(socketContext, type, reserved, data) {
    }

    Connect::~Connect() {
    }

    std::string Connect::protocol() {
        uint16_t protocolLength = be16toh(*reinterpret_cast<uint16_t*>(data.data() + 0));

        return std::string(data.data() + 2, protocolLength);
    }

    uint8_t Connect::version() {
        return static_cast<uint8_t>(*(data.data() + 6));
    }

    uint8_t Connect::flags() {
        return static_cast<uint8_t>(*(data.data() + 7));
    }

    uint16_t Connect::keepAlive() {
        return be16toh(*reinterpret_cast<uint16_t*>(data.data() + 8));
    }

    uint8_t Connect::propertyLength() {
        return static_cast<uint8_t>(*(data.data() + 10));
    }

    uint8_t Connect::expiryIntervalIdentifier() {
        return static_cast<uint8_t>(*(data.data() + 11));
    }

    uint64_t Connect::expiryInterval() {
        return be64toh(*reinterpret_cast<uint64_t*>(data.data() + 12));
    }

} // namespace mqtt::packets
