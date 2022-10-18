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

#include "iot/mqtt/packets/Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Unsubscribe::Unsubscribe()
        : iot::mqtt::ControlPacket(MQTT_UNSUBSCRIBE) {
    }

    Unsubscribe::Unsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics)
        : iot::mqtt::ControlPacket(MQTT_UNSUBSCRIBE) {
        this->packetIdentifier = packetIdentifier;
        this->topics = topics;
    }

    std::vector<char> Unsubscribe::serializeVP() const {
        std::vector<char> packet(packetIdentifier.serialize());

        for (const std::string& topic : topics) {
            iot::mqtt::types::String topicString;
            topicString = topic;

            std::vector<char> tmpPacket = topicString.serialize();
            packet.insert(packet.end(), tmpPacket.begin(), tmpPacket.end());
        }

        return packet;
    }

    uint16_t Unsubscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<std::string>& Unsubscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt::packets
