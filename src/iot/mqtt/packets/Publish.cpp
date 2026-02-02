/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "iot/mqtt/packets/Publish.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::packets {

    Publish::Publish()
        : iot::mqtt::ControlPacket(MQTT_PUBLISH, "PUBLISH") {
    }

    Publish::Publish(uint16_t packetIdentifier,
                     const std::string& topic,
                     const std::string& message,
                     uint8_t qoS,
                     bool dup,
                     bool retain,
                     bool includeProperties)
        : Publish() {
        this->flags = static_cast<uint8_t>((dup ? 0x08 : 0x00) | ((qoS << 1) & 0x06) | (retain ? 0x01 : 0x00));
        this->packetIdentifier = packetIdentifier;
        this->topic = topic;
        this->message = message;
        this->dup = dup;
        this->qoS = qoS;
        this->retain = retain;
        this->includeProperties = includeProperties;
        this->propertiesLength = 0;
    }

    std::vector<char> Publish::serializeVP() const {
        std::vector<char> packet(topic.serialize());

        if (qoS > 0) {
            std::vector<char> tmpVector = packetIdentifier.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }

        if (includeProperties) {
            std::vector<char> tmpVector = propertiesLength.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }

        std::vector<char> tmpVector = message.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    bool Publish::getDup() const {
        return dup;
    }

    uint8_t Publish::getQoS() const {
        return qoS;
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

} // namespace iot::mqtt::packets
