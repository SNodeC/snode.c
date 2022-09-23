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

#include "iot/mqtt/packets/Connack.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connack::Connack(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        flags = *reinterpret_cast<uint8_t*>(data.data() + pointer);
        pointer += 1;

        reason = *reinterpret_cast<uint8_t*>(data.data() + pointer);
    }

    Connack::Connack(uint8_t reason, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK, 0)
        , flags(flags)
        , reason(reason) {
        data.push_back(static_cast<char>(this->flags));  // Connack Flags: 0x00 - LSB is session present
        data.push_back(static_cast<char>(this->reason)); // Connack Reason: 0x00 = Success
    }

    Connack::~Connack() {
    }

    uint8_t Connack::getFlags() const {
        return flags;
    }

    uint8_t Connack::getReason() const {
        return reason;
    }

} // namespace iot::mqtt::packets
