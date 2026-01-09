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

#include "iot/mqtt/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::packets {

    Connect::Connect()
        : iot::mqtt::ControlPacket(MQTT_CONNECT, "CONNECT") {
    }

    Connect::Connect(const std::string& clientId,
                     uint16_t keepAlive,
                     bool cleanSession,
                     const std::string& willTopic,
                     const std::string& willMessage,
                     uint8_t willQoS,
                     bool willRetain,
                     const std::string& username,
                     const std::string& password,
                     bool loopPrevention)
        : Connect() {
        this->protocol = "MQTT";
        this->level =
            MQTT_VERSION_3_1_1 | (loopPrevention ? 0x80 : 0x00); // msb 1 -> do not reflect messages to origin (try_private in mosquitto)
        this->keepAlive = keepAlive;
        this->clientId = clientId;
        this->willTopic = willTopic;
        this->willMessage = willMessage;
        this->willQoS = willQoS;
        this->willRetain = willRetain;
        this->willFlag = !willMessage.empty();
        this->username = username;
        this->usernameFlag = !username.empty();
        this->password = password;
        this->passwordFlag = !password.empty();

        this->connectFlags = static_cast<uint8_t>((!usernameFlag ? 0 : 0x80) | (!passwordFlag ? 0 : 0x40) | (!willRetain ? 0 : 0x20) |
                                                  ((willQoS << 3) & 0x18) | (!willFlag ? 0 : 0x04) | (!cleanSession ? 0 : 0x02));
    }

    std::vector<char> Connect::serializeVP() const {
        std::vector<char> packet(protocol.serialize());

        std::vector<char> tmpVector = level.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = connectFlags.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = keepAlive.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = clientId.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        if (willFlag) {
            tmpVector = willTopic.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

            tmpVector = willMessage.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }
        if (usernameFlag) {
            tmpVector = username.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }
        if (passwordFlag) {
            tmpVector = password.serialize();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }

        return packet;
    }

    std::string Connect::getProtocol() const {
        return protocol;
    }

    uint8_t Connect::getLevel() const {
        return level;
    }

    uint8_t Connect::getConnectFlags() const {
        return connectFlags;
    }

    uint16_t Connect::getKeepAlive() const {
        return keepAlive;
    }

    std::string Connect::getClientId() const {
        return clientId;
    }

    bool Connect::getUsernameFlag() const {
        return usernameFlag;
    }

    bool Connect::getPasswordFlag() const {
        return passwordFlag;
    }

    bool Connect::getWillRetain() const {
        return willRetain;
    }

    uint8_t Connect::getWillQoS() const {
        return willQoS;
    }

    bool Connect::getWillFlag() const {
        return willFlag;
    }

    bool Connect::getCleanSession() const {
        return cleanSession;
    }

    bool Connect::getReflect() const {
        return reflect;
    }

    std::string Connect::getWillTopic() const {
        return willTopic;
    }

    std::string Connect::getWillMessage() const {
        return willMessage;
    }

    std::string Connect::getUsername() const {
        return username;
    }

    std::string Connect::getPassword() const {
        return password;
    }

} // namespace iot::mqtt::packets
