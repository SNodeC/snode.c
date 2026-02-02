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

#ifndef IOT_MQTT_PACKETS_CONNACK_H
#define IOT_MQTT_PACKETS_CONNACK_H

#include "iot/mqtt/ControlPacket.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"   // IWYU pragma: export
#include "iot/mqtt/types/UIntV.h"   // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MQTT_CONNACK_ACCEPT 0
#define MQTT_CONNACK_UNACEPTABLEVERSION 1
#define MQTT_CONNACK_IDENTIFIERREJECTED 2
#define MQTT_CONNACK_SERVERUNAVAILABLE 3
#define MQTT_CONNACK_BADUSERNAMEORPASSWORD 4
#define MQTT_CONNACK_NOTAUTHORIZED 5

#define MQTT_SESSION_NEW 0x00
#define MQTT_SESSION_PRESENT 0x01 // IWYU pragma: export

namespace iot::mqtt::packets {

    class Connack : public iot::mqtt::ControlPacket {
    public:
        Connack();
        Connack(uint8_t returncode, uint8_t acknowledgeFlags, bool includeProperties = false);

    private:
        std::vector<char> serializeVP() const override;

    public:
        uint8_t getAcknowledgeFlags() const;
        uint8_t getReturnCode() const;

        bool getSessionPresent() const;
        bool hasProperties() const;
        uint32_t getPropertiesLength() const;

    protected:
        iot::mqtt::types::UInt8 acknowledgeFlags;
        iot::mqtt::types::UInt8 returnCode;
        iot::mqtt::types::UIntV propertiesLength;

        bool sessionPresent = false;
        bool includeProperties = false;
        bool hasPropertiesFlag = false;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_CONNACK_H
