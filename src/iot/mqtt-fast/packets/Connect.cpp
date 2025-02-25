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

#include "iot/mqtt-fast/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Connect::Connect(const std::string& clientId, const std::string& protocol, uint8_t version, uint8_t flags, uint16_t keepAlive)
        : iot::mqtt_fast::ControlPacket(MQTT_CONNECT)
        , protocol(protocol)
        , level(version)
        , flags(flags)
        , keepAlive(keepAlive)
        , clientId(clientId) {
        // V-Header
        putString(this->protocol);
        putInt8(this->level);
        putInt8(this->flags);
        putInt16(this->keepAlive);

        // Payload
        putString(this->clientId);
    }

    Connect::Connect(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        // V-Header
        protocol = getString();
        level = getInt8();
        flags = getInt8();
        keepAlive = getInt16();

        usernameFlag = (flags & 0x80) != 0;
        passwordFlag = (flags & 0x40) != 0;
        willRetain = (flags & 0x20) != 0;
        willQoS = (flags & 0x18) >> 3;
        willFlag = (flags & 0x04) != 0;
        cleanSession = (flags & 0x02) != 0;
        reserved = (flags & 0x01) != 0;

        // Payload
        if (!isError()) {
            clientId = getString();
        } else {
            error = true;
        }

        if (!error && willFlag) {
            willTopic = getString();
            willMessage = getString();
        }

        if (!error && usernameFlag) {
            username = getString();
        }

        if (!error && passwordFlag) {
            password = getString();
        }
    }

    std::string Connect::getProtocol() const {
        return protocol;
    }

    uint8_t Connect::getLevel() const {
        return level;
    }

    uint8_t Connect::getFlags() const {
        return flags;
    }

    uint16_t Connect::getKeepAlive() const {
        return keepAlive;
    }

    const std::string& Connect::getClientId() const {
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

    const std::string& Connect::getWillTopic() const {
        return willTopic;
    }

    const std::string& Connect::getWillMessage() const {
        return willMessage;
    }

    const std::string& Connect::getUsername() const {
        return username;
    }

    const std::string& Connect::getPassword() const {
        return password;
    }

} // namespace iot::mqtt_fast::packets
