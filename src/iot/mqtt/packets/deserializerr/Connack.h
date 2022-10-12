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

#ifndef IOT_MQTT_PACKETS_DESERIALIZER_CONNACK_H
#define IOT_MQTT_PACKETS_DESERIALIZER_CONNACK_H

#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/packets/Connack.h" // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::deserializer {

    class Connack
        : public iot::mqtt::packets::Connack
        , public iot::mqtt::ControlPacketDeserializer {
    public:
        Connack(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::SocketContext* socketContext) override;
        void propagateEvent(iot::mqtt::SocketContext* socketContext) override;

        int state = 0;
    };

} // namespace iot::mqtt::packets::deserializer

#endif // IOT_MQTT_PACKETS_DESERIALIZER_CONNACK_H
