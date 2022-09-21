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

#include "mqtt/packets/Publish.h"

#include "mqtt/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::packets {

    Publish::Publish(ControlPacketFactory& controlPacketFactory)
        : mqtt::ControlPacket(controlPacketFactory) {
        dup = (controlPacketFactory.getPacketFlags() & 0x04) != 0;
        qoSLevel = static_cast<uint8_t>((controlPacketFactory.getPacketFlags() & 0x03) >> 1);
        retain = (controlPacketFactory.getPacketFlags() & 0x01) != 0;

        uint32_t pointer = 0;
        uint16_t strLen = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        name = std::string(data.data() + pointer, strLen);
        pointer += strLen;

        if (qoSLevel != 0 && data.size() > pointer) {
            packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += 2;
        }

        message = std::string(data.data() + pointer, data.size() - pointer);
    }

    Publish::~Publish() {
    }

    bool Publish::getDup() const {
        return dup;
    }

    uint8_t Publish::getQoSLevel() const {
        return qoSLevel;
    }

    uint16_t Publish::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::string& Publish::getName() const {
        return name;
    }

    const std::string& Publish::getMessage() const {
        return message;
    }

} // namespace mqtt::packets
