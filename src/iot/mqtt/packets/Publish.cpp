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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Publish::Publish()
        : iot::mqtt::ControlPacket(MQTT_PUBLISH) {
    }

    Publish::Publish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain)
        : iot::mqtt::ControlPacket(MQTT_PUBLISH) {
        this->flags = (dup ? 0x04 : 0x00) | ((qoSLevel << 1) & 0x06) | (retain ? 0x01 : 0x00);
        this->packetIdentifier = packetIdentifier;
        this->topic = topic;
        this->message = message;
        this->dup = dup;
        this->qoSLevel = qoSLevel;
        this->retain = retain;
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

    std::string Publish::getTopic() const {
        return topic;
    }

    std::string Publish::getMessage() const {
        return message;
    }

    bool Publish::getRetain() const {
        return retain;
    }

    std::vector<char> Publish::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = topic.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        if (qoSLevel > 0) {
            tmpVector = packetIdentifier.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }

        tmpVector = message.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

} // namespace iot::mqtt::packets
