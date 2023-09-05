/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/client/packets/Pingresp.h"

#include "iot/mqtt/client/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client::packets {

    Pingresp::Pingresp(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::client::ControlPacketDeserializer(remainingLength, flags, MQTT_PINGRESP_FLAGS) {
        this->flags = flags;
    }

    std::size_t Pingresp::deserializeVP([[maybe_unused]] iot::mqtt::MqttContext* mqttContext) {
        // no V-Header
        // no Payload

        complete = true;
        return 0;
    }

    void Pingresp::deliverPacket(iot::mqtt::client::Mqtt* mqtt) {
        mqtt->_onPingresp(*this);
    }

} // namespace iot::mqtt::client::packets
