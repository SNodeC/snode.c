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

#ifndef IOT_MQTT_PACKETSNEW_CONNECT_H
#define IOT_MQTT_PACKETSNEW_CONNECT_H

#include "iot/mqtt1/ControlPacket.h"
#include "iot/mqtt1/types/String.h" // IWYU pragma: export
#include "iot/mqtt1/types/UInt16.h" // IWYU pragma: export
#include "iot/mqtt1/types/UInt8.h"  // IWYU pragma: export

namespace iot::mqtt1 {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_CONNECT 0x01

#define MQTT_VERSION_3_1_1 0x04

namespace iot::mqtt1::packets {

    class Connect : public iot::mqtt1::ControlPacket {
    public:
        explicit Connect(const std::string& clientId);
        explicit Connect(uint32_t remainingLength, uint8_t reserved);

    private:
        std::size_t deserializeVP(iot::mqtt1::SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) const override;

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

        std::string getWillTopic() const;
        std::string getWillMessage() const;
        std::string getUsername() const;
        std::string getPassword() const;

    private:
        iot::mqtt1::types::String protocol;
        iot::mqtt1::types::UInt8 level;
        iot::mqtt1::types::UInt8 connectFlags;
        iot::mqtt1::types::UInt16 keepAlive;
        iot::mqtt1::types::String clientId;
        iot::mqtt1::types::String willTopic;
        iot::mqtt1::types::String willMessage;
        iot::mqtt1::types::String username;
        iot::mqtt1::types::String password;

        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;
        bool reserved = false;

        int state = 0;
    };

} // namespace iot::mqtt1::packets

#endif // IOT_MQTT_PACKETSNEW_CONNECT_H
