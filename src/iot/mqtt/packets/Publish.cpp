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

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Publish::Publish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain)
        : iot::mqtt::ControlPacket(MQTT_PUBLISH, (dup ? 0x04 : 0x00) | ((qoSLevel << 1) & 0x06) | (retain ? 0x01 : 0x00), 0) {
        this->packetIdentifier = packetIdentifier;
        this->topic = topic;
        this->message = message;
        this->dup = dup;
        this->qoSLevel = qoSLevel;
        this->retain = retain;
    }

    Publish::Publish(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt::ControlPacket(MQTT_PUBLISH, reserved, remainingLength) {
        this->qoSLevel = reserved >> 1 & 0x03;
        this->dup = (reserved & 0x04) != 0;
        this->retain = (reserved & 0x01) != 0;

        error = this->qoSLevel > 2;
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

    std::size_t Publish::deserializeVP(SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed += topic.deserialize(socketContext);

                if ((error = topic.isError()) || !topic.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                if (qoSLevel > 0) {
                    consumed += packetIdentifier.deserialize(socketContext);

                    if ((error = packetIdentifier.isError()) || !packetIdentifier.isComplete()) {
                        break;
                    } else if (packetIdentifier == 0) {
                        socketContext->close();
                        break;
                    }
                }

                message.setSize(static_cast<uint16_t>(getRemainingLength() - getConsumed() - consumed));

                state++;
                [[fallthrough]];
            case 2:
                consumed += message.deserialize(socketContext);

                if ((error = message.isError()) || !message.isComplete()) {
                    break;
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Publish::propagateEvent([[maybe_unused]] SocketContext* socketContext) {
        socketContext->_onPublish(*this);
    }

} // namespace iot::mqtt::packets
