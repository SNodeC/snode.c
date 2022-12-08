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

#include "iot/mqtt/client/packets/Connack.h"

#include "iot/mqtt/client/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client::packets {

    Connack::Connack(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::client::ControlPacketDeserializer(remainingLength, flags, MQTT_CONNACK_FLAGS) {
        this->flags = flags;
    }

    std::size_t Connack::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += acknowledgeFlags.deserialize(mqttContext);
                if (!acknowledgeFlags.isComplete()) {
                    break;
                }

                sessionPresent = acknowledgeFlags & 0x01;

                state++;
                [[fallthrough]];
            case 1:
                consumed += returnCode.deserialize(mqttContext);

                if (!returnCode.isComplete()) {
                    break;
                }

                complete = true;
                break;

                // no Payload
        }

        return consumed;
    }

    void Connack::propagateEvent(iot::mqtt::client::Mqtt* socketContext) {
        socketContext->_onConnack(*this);
    }

} // namespace iot::mqtt::client::packets
