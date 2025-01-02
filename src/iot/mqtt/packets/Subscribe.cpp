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

#include "iot/mqtt/packets/Subscribe.h"

#include "iot/mqtt/types/String.h"
#include "iot/mqtt/types/UInt16.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::packets {

    Subscribe::Subscribe()
        : iot::mqtt::ControlPacket(MQTT_SUBSCRIBE, "SUBSCRIBE") {
    }

    Subscribe::Subscribe(uint16_t packetIdentifier, const std::list<Topic>& topics)
        : Subscribe() {
        this->packetIdentifier = packetIdentifier;
        this->topics = topics;
    }

    std::vector<char> Subscribe::serializeVP() const {
        std::vector<char> packet(packetIdentifier.serialize());

        for (const Topic& topic : topics) {
            iot::mqtt::types::String topicString;
            topicString = topic.getName();

            std::vector<char> tmpPacket = topicString.serialize();
            packet.insert(packet.end(), tmpPacket.begin(), tmpPacket.end());

            packet.push_back(static_cast<char>(topic.getQoS()));
        }

        return packet;
    }

    uint16_t Subscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<Topic>& Subscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt::packets
