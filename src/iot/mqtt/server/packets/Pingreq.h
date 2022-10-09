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

#ifndef IOT_MQTT_PACKETSNEW_PINGREQ_H
#define IOT_MQTT_PACKETSNEW_PINGREQ_H

#include "iot/mqtt/ControlPacketReceiver.h" // IWYU pragma: export
#include "iot/mqtt/ControlPacketSender.h"   // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_PINGREQ 0x0C
#define MQTT_PINGREQ_FLAGS 0x00

namespace iot::mqtt::packets {

    class Pingreq
        : public iot::mqtt::ControlPacketReceiver
        , public iot::mqtt::ControlPacketSender {
    public:
        explicit Pingreq();                                        // Client
        explicit Pingreq(uint32_t remainingLength, uint8_t flags); // Server

    private:
        std::size_t deserializeVP(SocketContext* socketContext) override; // Server
        std::vector<char> serializeVP() const override;                   // Client
        void propagateEvent(SocketContext* socketContext) override;       // Server
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETSNEW_PINGREQ_H
