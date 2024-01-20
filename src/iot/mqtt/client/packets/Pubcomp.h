/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBCOMP_H
#define IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBCOMP_H

#include "iot/mqtt/client/ControlPacketDeserializer.h" // IWYU pra gma: export
#include "iot/mqtt/packets/Pubcomp.h"                  // IWYU pra gma: export

namespace iot::mqtt {
    class MqttContext;
    namespace client {
        class Mqtt;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client::packets {

    class Pubcomp
        : public iot::mqtt::packets::Pubcomp
        , public iot::mqtt::client::ControlPacketDeserializer {
    public:
        Pubcomp(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::MqttContext* mqttContext) override;
        void deliverPacket(iot::mqtt::client::Mqtt* mqtt) override;
    };

} // namespace iot::mqtt::client::packets

#endif // IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBREC_H
