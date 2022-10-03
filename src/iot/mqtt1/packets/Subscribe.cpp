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

#include "iot/mqtt1/packets/Subscribe.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::packets {

    Subscribe::Subscribe(uint16_t packetIdentifier, std::list<Topic>& topics)
        : iot::mqtt1::ControlPacket(MQTT_SUBSCRIBE, 0x02, 0) {
        this->packetIdentifier.setValue(packetIdentifier);
        this->topics = topics;
    }

    Subscribe::Subscribe(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt1::ControlPacket(MQTT_SUBSCRIBE, 0x02 | reserved, remainingLength) {
    }

    uint16_t Subscribe::getPacketIdentifier() const {
        return packetIdentifier.getValue();
    }

    const std::list<iot::mqtt1::Topic>& Subscribe::getTopics() const {
        return topics;
    }

    std::vector<char> Subscribe::getPacket() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = packetIdentifier.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        for (const Topic& topic : topics) {
            packet.insert(packet.end(), topic.getName().begin(), topic.getName().end());
            packet.push_back(static_cast<char>(topic.getRequestedQoS()));
        }

        return packet;
    }

    std::size_t Subscribe::construct(SocketContext* socketContext) {
        std::size_t consumedTotal = 0;
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = packetIdentifier.construct(socketContext);
                consumedTotal += consumed;

                if ((error = packetIdentifier.isError()) || !packetIdentifier.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 1:
                consumed = topic.construct(socketContext);
                consumedTotal += consumed;

                if ((error = topic.isError()) || !topic.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 2:
                consumed = qoS.construct(socketContext);
                consumedTotal += consumed;

                if ((error = qoS.isError()) || !qoS.isComplete()) {
                    break;
                }

                topics.push_back(Topic(topic.getValue(), qoS.getValue()));
                topic.reset();
                qoS.reset();

                if (getConsumed() + consumedTotal < this->getRemainingLength()) {
                    state = 1;
                    break;
                } else if (getConsumed() + consumedTotal > this->getRemainingLength()) {
                    error = true;
                    break;
                }
                [[fallthrough]];
            default:
                complete = true;
                break;
        }

        return consumedTotal;
    }

    void Subscribe::propagateEvent(SocketContext* socketContext) const {
        socketContext->_onSubscribe(*this);
    }

} // namespace iot::mqtt1::packets
