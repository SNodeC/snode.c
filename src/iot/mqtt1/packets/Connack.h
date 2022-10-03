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

#include "iot/mqtt1/ControlPacket.h"
#include "iot/mqtt1/types/UInt8.h"

namespace iot::mqtt1 {
    class SocketContext;
} // namespace iot::mqtt1

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint> // IWYU pragma: export
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_CONNACK 0x02

#define MQTT_CONNACK_ACCEPT 0
#define MQTT_CONNACK_UNACEPTABLEVERSION 1
#define MQTT_CONNACK_IDENTIFIERREJECTED 2
#define MQTT_CONNACK_SERVERUNAVAILABLE 3
#define MQTT_CONNACK_BADUSERNAMEORPASSWORD 4
#define MQTT_CONNACK_NOTAUTHORIZED 5

#define MQTT_SESSION_NEW 0x00
#define MQTT_SESSION_PRESENT 0x01

namespace iot::mqtt1::packets {

    class Connack : public iot::mqtt1::ControlPacket {
    public:
        Connack(uint8_t returncode, uint8_t flags);
        explicit Connack(uint32_t remainingLength, uint8_t reserved);

    private:
        std::size_t deserializeVP(iot::mqtt1::SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) const override;

    public:
        uint8_t getFlags() const;
        uint8_t getReturnCode() const;

    private:
        iot::mqtt1::types::UInt8 _flags;
        iot::mqtt1::types::UInt8 _returnCode;

        int state = 0;
    };

} // namespace iot::mqtt1::packets

#endif // IOT_MQTT_PACKETSNEW_CONNACK_H
