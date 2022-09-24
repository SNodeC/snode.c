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

#include "iot/mqtt/types/Binary.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connack::Connack(uint8_t reason, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK)
        , flags(flags)
        , reason(reason) {
        data.putInt8(this->flags);
        data.putInt8(this->reason);
    }

    Connack::Connack(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        flags = data.getInt8();
        reason = data.getInt8();

        error = data.isError();
    }

    uint8_t Connack::getFlags() const {
        return flags;
    }

    uint8_t Connack::getReason() const {
        return reason;
    }

} // namespace iot::mqtt::packets
