/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef IOT_MQTT_CONTROLPACKET_H
#define IOT_MQTT_CONTROLPACKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <vector>  // IWYU pragma: export

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MQTT_CONNECT 0x01
#define MQTT_CONNACK 0x02
#define MQTT_PUBLISH 0x03
#define MQTT_PUBACK 0x04
#define MQTT_PUBREC 0x05
#define MQTT_PUBREL 0x06
#define MQTT_PUBCOMP 0x07
#define MQTT_SUBSCRIBE 0x08
#define MQTT_SUBACK 0x09
#define MQTT_UNSUBSCRIBE 0x0A
#define MQTT_UNSUBACK 0x0B
#define MQTT_PINGREQ 0x0C
#define MQTT_PINGRESP 0x0D
#define MQTT_DISCONNECT 0x0E

#define MQTT_CONNECT_FLAGS 0x00
#define MQTT_CONNACK_FLAGS 0x00
// no flags for MQTT_PUBLISH_FLAGS
#define MQTT_PUBACK_FLAGS 0x00
#define MQTT_PUBREC_FLAGS 0x00
#define MQTT_PUBREL_FLAGS 0x02
#define MQTT_PUBCOMP_FLAGS 0x00
#define MQTT_SUBSCRIBE_FLAGS 0x02
#define MQTT_SUBACK_FLAGS 0x00
#define MQTT_UNSUBSCRIBE_FLAGS 0x02
#define MQTT_UNSUBACK_FLAGS 0x00
#define MQTT_PINGREQ_FLAGS 0x00
#define MQTT_PINGRESP_FLAGS 0x00
#define MQTT_DISCONNECT_FLAGS 0x00

namespace iot::mqtt {

    class ControlPacket {
    public:
        ControlPacket(uint8_t type);
        ControlPacket(const ControlPacket&) = default;

        virtual ~ControlPacket();

        ControlPacket& operator=(const ControlPacket&) = default;

    private:
        virtual std::vector<char> serializeVP() const = 0;

    public:
        std::vector<char> serialize() const;

        uint8_t getType() const;
        uint8_t getFlags() const;

    protected:
        uint8_t type = 0;
        uint8_t flags = 0;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_CONTROLPACKET_H
