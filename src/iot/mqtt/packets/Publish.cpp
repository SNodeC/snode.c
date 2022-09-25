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

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Publish::Publish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain)
        : iot::mqtt::ControlPacket(MQTT_PUBLISH, (dup ? 0x04 : 0x00) | (qoSLevel ? (qoSLevel << 1) & 0x03 : 0x00) | (retain ? 0x01 : 0x00))
        , packetIdentifier(packetIdentifier)
        , topic(topic)
        , message(message)
        , dup(dup)
        , qoSLevel(qoSLevel)
        , retain(retain) {
        // V-Header
        putString(this->topic);

        if (this->qoSLevel > 0) {
            putInt16(this->packetIdentifier);
        }

        // Payload
        putStringRaw(this->message);
    }

    Publish::Publish(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        dup = (controlPacketFactory.getPacketFlags() & 0x08) != 0;
        qoSLevel = static_cast<uint8_t>((controlPacketFactory.getPacketFlags() & 0x06) >> 1);
        retain = (controlPacketFactory.getPacketFlags() & 0x01) != 0;

        // V-Header
        topic = getString();

        if (qoSLevel != 0) {
            packetIdentifier = getInt16();
        }

        // Payload
        message = getStringRaw();

        error = isError();
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
