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

#ifndef IOT_MQTTFAST_PACKETS_PUBASK_H
#define IOT_MQTTFAST_PACKETS_PUBASK_H

#include "iot/mqtt-fast/ControlPacket.h"

namespace iot::mqtt_fast {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MQTT_PUBACK 0x04

namespace iot::mqtt_fast::packets {

    class Puback : public iot::mqtt_fast::ControlPacket {
    public:
        explicit Puback(uint16_t packetIdentifier);
        explicit Puback(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory);

        uint16_t getPacketIdentifier() const;

    private:
        uint16_t packetIdentifier;
    };

} // namespace iot::mqtt_fast::packets

#endif // IOT_MQTTFAST_PACKETS_PUBASK_H
