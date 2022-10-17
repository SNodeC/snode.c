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

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Suback::Suback()
        : iot::mqtt::ControlPacket(MQTT_SUBACK) {
    }

    Suback::Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes)
        : iot::mqtt::ControlPacket(MQTT_SUBACK) {
        this->packetIdentifier = packetIdentifier;
        this->returnCodes = returnCodes;
    }

    std::vector<char> Suback::serializeVP() const {
        std::vector<char> packet(packetIdentifier.serialize());

        packet.insert(packet.end(), returnCodes.begin(), returnCodes.end());

        return packet;
    }

    uint16_t Suback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<uint8_t>& Suback::getReturnCodes() const {
        return returnCodes;
    }

} // namespace iot::mqtt::packets
