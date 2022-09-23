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

#include <endian.h>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connect::Connect(std::string clientId, std::string protocol, uint8_t version, uint8_t flags, uint16_t keepAlive)
        : iot::mqtt::ControlPacket(MQTT_CONNECT, 0)
        , protocol(protocol)
        , version(version)
        , flags(flags)
        , keepAlive(keepAlive)
        , clientId(clientId) {
        std::string::size_type protocolLen = this->protocol.size();
        data.push_back(static_cast<char>(protocolLen >> 8 & 0xFF));
        data.push_back(static_cast<char>(protocolLen & 0XFF));
        data.insert(data.end(), this->protocol.begin(), this->protocol.end());

        data.push_back(static_cast<char>(this->version)); // Protocol Level (4)
        data.push_back(static_cast<char>(this->flags));   // Connect Flags (Clean Session)

        data.push_back(static_cast<char>(this->keepAlive >> 0x08 & 0xFF)); // Keep Alive MSB (60Sec)
        data.push_back(static_cast<char>(this->keepAlive & 0xFF));         // Keep Alive LSB

        // Payload
        std::string::size_type clientIdLen = this->clientId.size();
        data.push_back(static_cast<char>(clientIdLen >> 8 & 0xFF)); // Client ID Length MSB (2)
        data.push_back(static_cast<char>(clientIdLen & 0xFF));      // Client ID Length LSB
        data.insert(data.end(), this->clientId.begin(), this->clientId.end());
    }

    Connect::Connect(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        uint16_t protocolLength = be16toh(*reinterpret_cast<uint16_t*>(const_cast<char*>(data.data() + pointer)));
        pointer += 2;

        protocol = std::string(data.data() + pointer, protocolLength);
        pointer += protocolLength;

        version = static_cast<uint8_t>(*(data.data() + pointer));
        pointer += 1;

        flags = static_cast<uint8_t>(*(data.data() + pointer));
        pointer += 1;

        keepAlive = be16toh(*reinterpret_cast<uint16_t*>(const_cast<char*>(data.data() + pointer)));
    }

    std::string Connect::getProtocol() const {
        return protocol;
    }

    uint8_t Connect::getVersion() const {
        return version;
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

} // namespace iot::mqtt::packets
