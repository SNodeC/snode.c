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

#include "iot/mqtt-fast/packets/Suback.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Suback::Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes)
        : iot::mqtt_fast::ControlPacket(MQTT_SUBACK)
        , packetIdentifier(packetIdentifier)
        , returnCodes(std::move(returnCodes)) {
        // V-Header
        putInt16(this->packetIdentifier);

        // Payload
        putUint8ListRaw(returnCodes);
    }

    Suback::Suback(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        // V-Header
        packetIdentifier = getInt16();

        // Payload
        returnCodes = getUint8ListRaw();

        error = isError();
    }

    uint16_t Suback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<uint8_t>& Suback::getReturnCodes() const {
        return returnCodes;
    }

} // namespace iot::mqtt_fast::packets
