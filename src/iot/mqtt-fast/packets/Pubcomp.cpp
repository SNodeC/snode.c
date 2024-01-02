/*
 * SNode.C - a slim toolkit for network communication
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

#include "iot/mqtt-fast/packets/Pubcomp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Pubcomp::Pubcomp(uint16_t packetIdentifier)
        : iot::mqtt_fast::ControlPacket(MQTT_PUBCOMP)
        , packetIdentifier(packetIdentifier) {
        // V-Header
        putInt16(this->packetIdentifier);

        // no Payload
    }

    Pubcomp::Pubcomp(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        // V-Header
        packetIdentifier = getInt16();

        // no Payload

        error = isError();
    }

    uint16_t Pubcomp::getPacketIdentifier() const {
        return packetIdentifier;
    }

} // namespace iot::mqtt_fast::packets
