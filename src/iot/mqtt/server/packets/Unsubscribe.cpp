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

#include "iot/mqtt/server/packets/Unsubscribe.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::packets {

    Unsubscribe::Unsubscribe(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::server::ControlPacketDeserializer(remainingLength, flags, MQTT_UNSUBSCRIBE_FLAGS) {
        this->flags = flags;
    }

    std::size_t Unsubscribe::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += packetIdentifier.deserialize(mqttContext);
                if (!packetIdentifier.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1: // Payload
                consumed += topic.deserialize(mqttContext);

                if (!topic.isComplete()) {
                    break;
                } else {
                    topics.push_back(topic);
                    topic.reset();

                    if (getConsumed() + consumed < this->getRemainingLength()) {
                        break;
                    }
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Unsubscribe::propagateEvent(iot::mqtt::server::Mqtt* mqtt) {
        mqtt->_onUnsubscribe(*this);
    }

} // namespace iot::mqtt::server::packets
