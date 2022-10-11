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

#include "iot/mqtt/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connect::Connect()
        : iot::mqtt::ControlPacket(MQTT_CONNECT, MQTT_CONNECT_FLAGS) {
    }

    Connect::Connect(const std::string& clientId)
        : iot::mqtt::ControlPacket(MQTT_CONNECT, MQTT_CONNECT_FLAGS) {
        this->clientId = clientId;
    }

    std::vector<char> Connect::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = protocol.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = level.serialize();
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

    void Connect::setClientId(const std::string& clientId) {
        this->clientId = clientId;
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
