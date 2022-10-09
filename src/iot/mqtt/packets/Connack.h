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

#ifndef IOT_MQTT_PACKETSNEW_CONNACK_H
#define IOT_MQTT_PACKETSNEW_CONNACK_H

#include "iot/mqtt/ControlPacketReceiver.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"           // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_CONNACK 0x02
#define MQTT_CONNACK_FLAGS 0x00

#define MQTT_CONNACK_ACCEPT 0
#define MQTT_CONNACK_UNACEPTABLEVERSION 1
#define MQTT_CONNACK_IDENTIFIERREJECTED 2
#define MQTT_CONNACK_SERVERUNAVAILABLE 3
#define MQTT_CONNACK_BADUSERNAMEORPASSWORD 4
#define MQTT_CONNACK_NOTAUTHORIZED 5

#define MQTT_SESSION_NEW 0x00
#define MQTT_SESSION_PRESENT 0x01

namespace iot::mqtt::packets {

    class Connack : public iot::mqtt::ControlPacketReceiver {
    public:
        Connack() = default;
        explicit Connack(uint32_t remainingLength, uint8_t flags); // Client

    private:
        std::size_t deserializeVP(iot::mqtt::SocketContext* socketContext) override; // Client
        void propagateEvent(SocketContext* socketContext) override;                  // Client

    public:
        uint8_t getFlags() const;
        uint8_t getReturnCode() const;

        bool getSessionPresent() const;

    protected:
        iot::mqtt::types::UInt8 flags;
        iot::mqtt::types::UInt8 returnCode;

        bool sessionPresent = false;

        int state = 0;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETSNEW_CONNACK_H
