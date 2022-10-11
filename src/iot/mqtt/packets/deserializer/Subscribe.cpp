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

#include "iot/mqtt/packets/deserializer/Subscribe.h"

#include "iot/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::deserializer {

    Subscribe::Subscribe(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_SUBSCRIBE, flags)
        , iot::mqtt::ControlPacketDeserializer(remainingLength, MQTT_SUBSCRIBE_FLAGS) {
    }

    std::size_t Subscribe::deserializeVP(iot::mqtt::SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += packetIdentifier.deserialize(socketContext);

                if (!packetIdentifier.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1: // Payload
                consumed += topic.deserialize(socketContext);

                if (!topic.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 2:
                consumed += qoS.deserialize(socketContext);

                if (!qoS.isComplete()) {
                    break;
                } else {
                    topics.push_back(Topic(topic, qoS));
                    topic.reset();
                    qoS.reset();

                    if (getConsumed() + consumed < this->getRemainingLength()) {
                        state = 1;
                        break;
                    }
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Subscribe::propagateEvent(iot::mqtt::SocketContext* socketContext) {
        dynamic_cast<iot::mqtt::server::SocketContext*>(socketContext)->_onSubscribe(*this);
    }

} // namespace iot::mqtt::packets::deserializer
