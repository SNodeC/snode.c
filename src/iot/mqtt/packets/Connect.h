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

#ifndef IOT_MQTT_PACKETS_CONNECT_H
#define IOT_MQTT_PACKETS_CONNECT_H

#include "iot/mqtt/ControlPacket.h" // IWYU pragma: export
#include "iot/mqtt/types/String.h"  // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h"  // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"   // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MQTT_VERSION_3_1_1 0x04
#define MQTT_VERSION_5_0 0x05

namespace iot::mqtt::packets {

    class Connect : public iot::mqtt::ControlPacket {
    public:
        Connect();

        Connect(const std::string& clientId,
                uint16_t keepAlive,
                bool cleanSession,
                const std::string& willTopic,
                const std::string& willMessage,
                uint8_t willQoS,
                bool willRetain,
                const std::string& username,
                const std::string& password,
                bool loopPrevention);

    private:
        std::vector<char> serializeVP() const override;

    public:
        std::string getProtocol() const;
        uint8_t getLevel() const;
        uint8_t getConnectFlags() const;
        uint16_t getKeepAlive() const;

        std::string getClientId() const;

        bool getUsernameFlag() const;
        bool getPasswordFlag() const;
        bool getWillRetain() const;
        uint8_t getWillQoS() const;
        bool getWillFlag() const;
        bool getCleanSession() const;
        bool getReflect() const;

        std::string getWillTopic() const;
        std::string getWillMessage() const;
        std::string getUsername() const;
        std::string getPassword() const;

    protected:
        iot::mqtt::types::String protocol;
        iot::mqtt::types::UInt8 level;
        iot::mqtt::types::UInt8 connectFlags;
        iot::mqtt::types::UInt16 keepAlive;
        iot::mqtt::types::String clientId;
        iot::mqtt::types::String willTopic;
        iot::mqtt::types::String willMessage;
        iot::mqtt::types::String username;
        iot::mqtt::types::String password;

        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;
        bool reflect = true;
        bool reserved = false;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_CONNECT_H
