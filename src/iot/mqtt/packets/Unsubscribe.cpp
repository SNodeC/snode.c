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

#include <endian.h>
#include <utility>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Unsubscribe::Unsubscribe(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        while (data.size() > pointer) {
            uint16_t strLen = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += 2;

            std::string name = std::string(data.data() + pointer, strLen);
            pointer += strLen;

            topics.push_back(name);
        }
    }

    Unsubscribe::Unsubscribe(uint16_t packetIdentifier, const std::list<std::string>& topics)
        : iot::mqtt::ControlPacket(MQTT_UNSUBSCRIBE, 0x02)
        , packetIdentifier(packetIdentifier)
        , topics(std::move(topics)) {
        data.push_back(static_cast<char>(this->packetIdentifier >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(this->packetIdentifier & 0xFF));

        for (std::string& topic : this->topics) {
            uint16_t topicLen = static_cast<uint16_t>(topic.size());

            data.push_back(static_cast<char>(topicLen >> 0x08 & 0xFF));
            data.push_back(static_cast<char>(topicLen & 0xFF));

            data.insert(data.end(), topic.begin(), topic.end());
        }
    }

    Unsubscribe::~Unsubscribe() {
    }

    uint16_t Unsubscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<std::string>& Unsubscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt::packets
