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

#include "iot/mqtt/ControlPacket.h"

#include "iot/mqtt/StaticHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    ControlPacket::ControlPacket(uint8_t type)
        : type(type) {
        switch (type) {
            case MQTT_CONNECT:
                flags = MQTT_CONNECT_FLAGS;
                break;
            case MQTT_CONNACK:
                flags = MQTT_CONNACK_FLAGS;
                break;
            case MQTT_PUBLISH:
                // no value for MQTT_PUBLISH_FLAGS
                break;
            case MQTT_PUBACK:
                flags = MQTT_PUBACK_FLAGS;
                break;
            case MQTT_PUBREC:
                flags = MQTT_PUBREC_FLAGS;
                break;
            case MQTT_PUBREL:
                flags = MQTT_PUBREL_FLAGS;
                break;
            case MQTT_PUBCOMP:
                flags = MQTT_PUBCOMP_FLAGS;
                break;
            case MQTT_SUBSCRIBE:
                flags = MQTT_SUBSCRIBE_FLAGS;
                break;
            case MQTT_SUBACK:
                flags = MQTT_SUBACK_FLAGS;
                break;
            case MQTT_UNSUBSCRIBE:
                flags = MQTT_UNSUBSCRIBE_FLAGS;
                break;
            case MQTT_UNSUBACK:
                flags = MQTT_UNSUBACK_FLAGS;
                break;
            case MQTT_PINGREQ:
                flags = MQTT_PINGREQ_FLAGS;
                break;
            case MQTT_PINGRESP:
                flags = MQTT_PINGRESP_FLAGS;
                break;
            case MQTT_DISCONNECT:
                flags = MQTT_DISCONNECT_FLAGS;
                break;
        }
    }

    std::vector<char> ControlPacket::serialize() const {
        std::vector<char> variablHeaderPayload = serializeVP();

        iot::mqtt::StaticHeader staticHeader(getType(), getFlags());
        staticHeader.setRemainingLength(static_cast<uint32_t>(variablHeaderPayload.size()));

        std::vector<char> packet = staticHeader.serialize();

        packet.insert(packet.end(), variablHeaderPayload.begin(), variablHeaderPayload.end());

        return packet;
    }

    uint8_t ControlPacket::getType() const {
        return type;
    }

    uint8_t ControlPacket::getFlags() const {
        return flags;
    }

} // namespace iot::mqtt
