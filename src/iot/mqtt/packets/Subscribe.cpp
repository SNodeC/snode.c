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

#include "iot/mqtt/packets/Subscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <string>
#include <utility>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Subscribe::Subscribe(uint16_t packetIdentifier, const std::list<Topic>& topics)
        : iot::mqtt::ControlPacket(MQTT_SUBSCRIBE, 0x02)
        , packetIdentifier(packetIdentifier)
        , topics(std::move(topics)) {
        data.push_back(static_cast<char>(this->packetIdentifier >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(this->packetIdentifier & 0x08));

        for (const iot::mqtt::Topic& topic : this->topics) {
            uint16_t topicLen = static_cast<uint16_t>(topic.getName().size());
            data.push_back(static_cast<char>(topicLen >> 0x08 & 0xFF));
            data.push_back(static_cast<char>(topicLen & 0x08));

            data.insert(data.end(), topic.getName().begin(), topic.getName().end());
            data.push_back(static_cast<char>(topic.getRequestedQos())); // Topic QoS
        }
    }

    Subscribe::Subscribe(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        while (data.size() > pointer) {
            uint16_t strLen = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += 2;

            std::string name = std::string(data.data() + pointer, strLen);
            pointer += strLen;

            uint8_t requestedQos = *reinterpret_cast<uint8_t*>(data.data() + pointer);
            pointer += 1;

            topics.push_back(mqtt::Topic(name, requestedQos));
        }
    }

    uint16_t Subscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<iot::mqtt::Topic>& Subscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt::packets
