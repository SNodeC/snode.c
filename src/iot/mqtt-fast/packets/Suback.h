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

#ifndef IOT_MQTTFAST_PACKETS_SUBACK_H
#define IOT_MQTTFAST_PACKETS_SUBACK_H

#include "iot/mqtt-fast/ControlPacket.h"

namespace iot::mqtt_fast {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MQTT_SUBACK 0x09

namespace iot::mqtt_fast::packets {

    class Suback : public iot::mqtt_fast::ControlPacket {
    public:
        Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes);
        explicit Suback(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory);

        uint16_t getPacketIdentifier() const;

        const std::list<uint8_t>& getReturnCodes() const;

    private:
        uint16_t packetIdentifier;
        std::list<uint8_t> returnCodes;
    };

} // namespace iot::mqtt_fast::packets

#endif // IOT_MQTTFAST_PACKETS_SUBACK_H
