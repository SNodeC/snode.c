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

#ifndef IOT_MQTT_PACKETS_PUBASK_H
#define IOT_MQTT_PACKETS_PUBASK_H

#include "iot/mqtt/ControlPacketReceiver.h" // IWYU pragma: export
#include "iot/mqtt/ControlPacketSender.h"   // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h"          // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_PUBACK 0x04
#define MQTT_PUBACK_FLAGS 0x00

namespace iot::mqtt::packets {

    class Puback
        : public iot::mqtt::ControlPacketReceiver
        , public iot::mqtt::ControlPacketSender {
    public:
        explicit Puback(const uint16_t packetIdentifier);
        explicit Puback(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) override;

    public:
        uint16_t getPacketIdentifier() const;

    private:
        iot::mqtt::types::UInt16 packetIdentifier;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_PUBASK_H
