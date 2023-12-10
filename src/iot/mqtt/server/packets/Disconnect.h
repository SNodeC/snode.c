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

#ifndef IOT_MQTT_SERVER_PACKETS_DESERIALIZER_DISCONNECT_H
#define IOT_MQTT_SERVER_PACKETS_DESERIALIZER_DISCONNECT_H

#include "iot/mqtt/packets/Disconnect.h"               // IWYU pragma: export
#include "iot/mqtt/server/ControlPacketDeserializer.h" // IWYU pragma: export

namespace iot::mqtt {
    class MqttContext;
    namespace server {
        class Mqtt;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::packets {

    class Disconnect
        : public iot::mqtt::packets::Disconnect
        , public iot::mqtt::server::ControlPacketDeserializer {
    public:
        Disconnect(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::MqttContext* mqttContext) override;
        void deliverPacket(iot::mqtt::server::Mqtt* mqtt) override;
    };

} // namespace iot::mqtt::server::packets

#endif // IOT_MQTT_SERVER_PACKETS_DESERIALIZER_DISCONNECT_H
