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

#include "iot/mqtt/client/packets/Pubcomp.h"

#include "iot/mqtt/client/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client::packets {

    Pubcomp::Pubcomp(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::client::ControlPacketDeserializer(remainingLength, flags, MQTT_PUBCOMP_FLAGS) {
        this->flags = flags;
    }

    std::size_t Pubcomp::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        // V-Header
        const std::size_t consumed = packetIdentifier.deserialize(mqttContext);
        complete = packetIdentifier.isComplete();

        // no Payload

        return consumed;
    }

    void Pubcomp::deliverPacket(iot::mqtt::client::Mqtt* mqtt) {
        mqtt->printVP(*this);
        mqtt->_onPubcomp(*this);
    }

} // namespace iot::mqtt::client::packets
