/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "iot/mqtt-fast/packets/Publish.h"

#include "iot/mqtt-fast/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Publish::Publish(uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoS, bool retain)
        : iot::mqtt_fast::ControlPacket(MQTT_PUBLISH,
                                        static_cast<uint8_t>((dup ? 0x04 : 0x00) | ((qoS << 1) & 0x06) | (retain ? 0x01 : 0x00)))
        , packetIdentifier(packetIdentifier)
        , topic(topic)
        , message(message)
        , dup(dup)
        , qoS(qoS)
        , retain(retain) {
        // V-Header
        putString(this->topic);

        if (this->qoS > 0) {
            putInt16(this->packetIdentifier);
        }

        // Payload
        putStringRaw(this->message);
    }

    Publish::Publish(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        dup = (controlPacketFactory.getPacketFlags() & 0x08) != 0;
        qoS = static_cast<uint8_t>((controlPacketFactory.getPacketFlags() & 0x06) >> 1);
        retain = (controlPacketFactory.getPacketFlags() & 0x01) != 0;

        // V-Header
        topic = getString();

        if (qoS != 0) {
            packetIdentifier = getInt16();
        }

        // Payload
        message = getStringRaw();

        error = isError();
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

    const std::string& Publish::getTopic() const {
        return topic;
    }

    const std::string& Publish::getMessage() const {
        return message;
    }

    bool Publish::getRetain() const {
        return retain;
    }

} // namespace iot::mqtt_fast::packets
