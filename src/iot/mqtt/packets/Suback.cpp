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

#include "iot/mqtt/packets/Suback.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <utility>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Suback::Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes)
        : iot::mqtt::ControlPacket(MQTT_SUBACK)
        , packetIdentifier(packetIdentifier)
        , returnCodes(std::move(returnCodes)) {
        data.push_back(static_cast<char>(this->packetIdentifier >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(this->packetIdentifier & 0xFF));

        for (uint8_t returnCode : this->returnCodes) {
            data.push_back(static_cast<char>(returnCode));
        }
    }

    Suback::Suback(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        std::vector<char>::size_type pointer = 0;

        packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        for (; pointer < this->getRemainingLength(); ++pointer) {
            returnCodes.push_back(static_cast<uint8_t>(*(data.data() + pointer)));
        }
    }

    uint16_t Suback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<uint8_t>& Suback::getReturnCodes() const {
        return returnCodes;
    }

} // namespace iot::mqtt::packets
