/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "iot/mqtt/packets/Pubcomp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::packets {

    Pubcomp::Pubcomp()
        : iot::mqtt::ControlPacket(MQTT_PUBCOMP, "PUBCOMP") {
    }

    Pubcomp::Pubcomp(const uint16_t packetIdentifier)
        : Pubcomp() {
        this->packetIdentifier = packetIdentifier;
    }

    std::vector<char> Pubcomp::serializeVP() const {
        return packetIdentifier.serialize();
    }

    uint16_t Pubcomp::getPacketIdentifier() const {
        return packetIdentifier;
    }

} // namespace iot::mqtt::packets
