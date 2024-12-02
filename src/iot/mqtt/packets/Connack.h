/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef IOT_MQTT_PACKETS_CONNACK_H
#define IOT_MQTT_PACKETS_CONNACK_H

#include "iot/mqtt/ControlPacket.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"   // IWYU pragma: export

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
        Connack(uint8_t returncode, uint8_t acknowledgeFlags);

    private:
        std::vector<char> serializeVP() const override;

    public:
        uint8_t getAcknowledgeFlags() const;
        uint8_t getReturnCode() const;

        bool getSessionPresent() const;

    protected:
        iot::mqtt::types::UInt8 acknowledgeFlags;
        iot::mqtt::types::UInt8 returnCode;

        bool sessionPresent = false;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_CONNACK_H
