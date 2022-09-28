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

#ifndef IOT_MQTT_PACKETS_CONNECT_H
#define IOT_MQTT_PACKETS_CONNECT_H

#include "iot/mqtt/ControlPacket.h"

namespace iot::mqtt {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <string>  // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_CONNECT 0x01

namespace iot::mqtt::packets {

    class Connect : public iot::mqtt::ControlPacket {
    public:
        explicit Connect(
            std::string clientId, std::string protocol = "MQTT", uint8_t version = 4, uint8_t flags = 0, uint16_t keepAlive = 0x003C);
        explicit Connect(iot::mqtt::ControlPacketFactory& controlPacketFactory);

        std::string getProtocol() const;
        uint8_t getVersion() const;
        uint8_t getFlags() const;
        uint16_t getKeepAlive() const;

        const std::string& getClientId() const;

        bool getUsernameFlag() const;
        bool getPasswordFlag() const;
        bool getWillRetain() const;
        uint8_t getWillQoS() const;
        bool getWillFlag() const;
        bool getCleanSession() const;

        const std::string& getWillTopic() const;
        const std::string& getWillMessage() const;
        const std::string& getUsername() const;
        const std::string& getPassword() const;

    private:
        std::string protocol;
        uint8_t version = 0;
        uint8_t flags = 0;
        uint16_t keepAlive = 0;

        std::string clientId;
        std::string willTopic;
        std::string willMessage;
        std::string username;
        std::string password;

        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;
        bool reserved = false;
    };

} // namespace iot::mqtt::packets

/*

V5
    VARIABLE HEADER
        2022-09-19 17:37:53 0000000012:   1.     - 0x00  Length MSB
        2022-09-19 17:37:53 0000000012:   2.     - 0x04  Length LSB
        2022-09-19 17:37:53 0000000012:   3.   M - 0x4d Protocol
        2022-09-19 17:37:53 0000000012:   4.   Q - 0x51
        2022-09-19 17:37:53 0000000012:   5.   T - 0x54
        2022-09-19 17:37:53 0000000012:   6.   T - 0x54

        2022-09-19 17:37:53 0000000012:   7.     - 0x05  Version
        2022-09-19 17:37:53 0000000012:   8.     - 0x02  Flags

        2022-09-19 17:37:53 0000000012:   9.     - 0x00  Keep Aliva MSB 0x003c = 60sec
        2022-09-19 17:37:53 0000000012:  10.   < - 0x3c  Keep Alive LSB

        2022-09-19 17:37:53 0000000012:  11.     - 0x03  Properties Length

        2022-09-19 17:37:53 0000000012:  12.   ! - 0x21 Property: Receive Maximum (2Byte)
        2022-09-19 17:37:53 0000000012:  13.     - 0x00  Receive Maximum MSB
        2022-09-19 17:37:53 0000000012:  14.     - 0x14  Receive Maximum LSB

    PAYLOAD
        2022-09-19 17:37:53 0000000012:  15.     - 0x00  ID Length MSB
        2022-09-19 17:37:53 0000000012:  16.     - 0x02  ID Length LSB
        2022-09-19 17:37:53 0000000012:  17.   d - 0x64 ID-Byte 1
        2022-09-19 17:37:53 0000000012:  18.   f - 0x66 ID-Byte 2

    WILL Properties (optional)
    WILL Topic (optional)
    User Name
    Password

 */

#endif // IOT_MQTT_PACKETS_CONNECT_H
