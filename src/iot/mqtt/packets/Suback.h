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

#ifndef IOT_MQTT_PACKETSNEW_SUBACK_H
#define IOT_MQTT_PACKETSNEW_SUBACK_H

#include "iot/mqtt/ControlPacket.h"
#include "iot/mqtt/types/UInt16.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"  // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_SUBACK 0x09

namespace iot::mqtt::packets {

    class Suback : public iot::mqtt::ControlPacket {
    public:
        Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes);
        explicit Suback(uint32_t remainingLength, uint8_t reserved);

    private:
        std::size_t deserializeVP(SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) override;

    public:
        uint16_t getPacketIdentifier() const;
        const std::list<uint8_t>& getReturnCodes() const;

    private:
        iot::mqtt::types::UInt16 packetIdentifier;
        iot::mqtt::types::UInt8 returnCode;
        std::list<uint8_t> returnCodes;

        int state;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETSNEW_SUBACK_H
