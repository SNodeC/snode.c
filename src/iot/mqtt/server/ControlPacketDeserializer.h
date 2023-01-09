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

#ifndef IOT_MQTT_SERVER_CONTROLPACKETDESERIALIZER_H
#define IOT_MQTT_SERVER_CONTROLPACKETDESERIALIZER_H

#include "iot/mqtt/ControlPacketDeserializer.h" // IWYU pragma: export

namespace iot::mqtt::server {
    class Mqtt;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    class ControlPacketDeserializer : public iot::mqtt::ControlPacketDeserializer {
    public:
        using iot::mqtt::ControlPacketDeserializer::ControlPacketDeserializer;

        virtual void deliverPacket(iot::mqtt::server::Mqtt* socketContext) = 0;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_CONTROLPACKETDESERIALIZER_H
