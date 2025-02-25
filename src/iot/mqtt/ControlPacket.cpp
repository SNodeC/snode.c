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

#include "iot/mqtt/ControlPacket.h"

#include "iot/mqtt/FixedHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    const std::vector<std::string> mqttPackageName = { //
        "Reserved",
        "CONNECT",
        "CONNACK",
        "PUBLISH",
        "PUBACK",
        "PUBREC",
        "PUBREL",
        "PUBCOMP",
        "SUBSCRIBE",
        "SUBACK",
        "UNSUBSCRIBE",
        "UNSUBACK",
        "PINGREQ",
        "PINGRESP",
        "DISCONNECT",
        "Reserved"};

    ControlPacket::ControlPacket(uint8_t type, const std::string& name)
        : name(name)
        , type(type) {
        switch (type) {
            case MQTT_CONNECT:
                flags = MQTT_CONNECT_FLAGS;
                break;
            case MQTT_CONNACK:
                flags = MQTT_CONNACK_FLAGS;
                break;
            case MQTT_PUBLISH:
                // no flags for MQTT_PUBLISH_FLAGS
                break;
            case MQTT_PUBACK:
                flags = MQTT_PUBACK_FLAGS;
                break;
            case MQTT_PUBREC:
                flags = MQTT_PUBREC_FLAGS;
                break;
            case MQTT_PUBREL:
                flags = MQTT_PUBREL_FLAGS;
                break;
            case MQTT_PUBCOMP:
                flags = MQTT_PUBCOMP_FLAGS;
                break;
            case MQTT_SUBSCRIBE:
                flags = MQTT_SUBSCRIBE_FLAGS;
                break;
            case MQTT_SUBACK:
                flags = MQTT_SUBACK_FLAGS;
                break;
            case MQTT_UNSUBSCRIBE:
                flags = MQTT_UNSUBSCRIBE_FLAGS;
                break;
            case MQTT_UNSUBACK:
                flags = MQTT_UNSUBACK_FLAGS;
                break;
            case MQTT_PINGREQ:
                flags = MQTT_PINGREQ_FLAGS;
                break;
            case MQTT_PINGRESP:
                flags = MQTT_PINGRESP_FLAGS;
                break;
            case MQTT_DISCONNECT:
                flags = MQTT_DISCONNECT_FLAGS;
                break;
        }
    }

    ControlPacket::~ControlPacket() {
    }

    const std::string& ControlPacket::getName() const {
        return name;
    }

    std::vector<char> ControlPacket::serialize() const {
        std::vector<char> variablHeaderPayload = serializeVP();

        const FixedHeader fixedHeader(type, flags, static_cast<uint32_t>(variablHeaderPayload.size()));

        std::vector<char> packet = fixedHeader.serialize();
        packet.insert(packet.end(), variablHeaderPayload.begin(), variablHeaderPayload.end());

        return packet;
    }

    uint8_t ControlPacket::getType() const {
        return type;
    }

    uint8_t ControlPacket::getFlags() const {
        return flags;
    }

} // namespace iot::mqtt
