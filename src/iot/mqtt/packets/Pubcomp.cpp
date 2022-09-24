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

#include "iot/mqtt/packets/Pubcomp.h"

#include "iot/mqtt/types/Binary.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Pubcomp::Pubcomp(uint16_t packetIdentifier)
        : iot::mqtt::ControlPacket(MQTT_PUBCOMP)
        , packetIdentifier(packetIdentifier) {
        data.putInt16(this->packetIdentifier);
    }

    Pubcomp::Pubcomp(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        packetIdentifier = data.getInt16();

        error = data.isError();
    }

    uint16_t Pubcomp::getPacketIdentifier() const {
        return packetIdentifier;
    }

} // namespace iot::mqtt::packets
