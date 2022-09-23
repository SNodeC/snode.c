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

#include "iot/mqtt/packets/Publish.h"

#include "iot/mqtt/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Publish::Publish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain)
        : iot::mqtt::ControlPacket(MQTT_PUBLISH, 0)
        , packetIdentifier(packetIdentifier)
        , topic(topic)
        , message(message)
        , dup(dup)
        , qoSLevel(qoSLevel)
        , retain(retain) {
        uint16_t topicLen = static_cast<uint16_t>(this->topic.size());
        data.push_back(static_cast<char>(topicLen >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(topicLen & 0xFF));
        data.insert(data.end(), this->topic.begin(), this->topic.end());

        if (qoSLevel > 0) {
            data.push_back(static_cast<char>(this->packetIdentifier >> 0x08 & 0xFF));
            data.push_back(static_cast<char>(this->packetIdentifier & 0xFF));
        }

        data.insert(data.end(), this->message.begin(), this->message.end());
    }

    Publish::Publish(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        dup = (controlPacketFactory.getPacketFlags() & 0x08) != 0;
        qoSLevel = static_cast<uint8_t>((controlPacketFactory.getPacketFlags() & 0x06) >> 1);
        retain = (controlPacketFactory.getPacketFlags() & 0x01) != 0;

        uint32_t pointer = 0;

        uint16_t strLen = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        topic = std::string(data.data() + pointer, strLen);
        pointer += strLen;

        if (qoSLevel != 0 && data.size() > pointer) {
            packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += 2;
        }

        message = std::string(data.data() + pointer, data.size() - pointer);
    }

    bool Publish::getDup() const {
        return dup;
    }

    uint8_t Publish::getQoSLevel() const {
        return qoSLevel;
    }

    uint16_t Publish::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::string& Publish::getTopic() const {
        return topic;
    }

    const std::string& Publish::getMessage() const {
        return message;
    }

    bool Publish::getRetain() const {
        return retain;
    }

} // namespace iot::mqtt::packets
