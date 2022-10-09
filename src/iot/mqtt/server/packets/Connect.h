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

#ifndef IOT_MQTT_SERVER_PACKETSNEW_CONNECT_H
#define IOT_MQTT_SERVER_PACKETSNEW_CONNECT_H

#include "iot/mqtt/ControlPacketReceiver.h" // IWYU pragma: export
#include "iot/mqtt/packets/Connect.h"
#include "iot/mqtt/types/String.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt8.h"  // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_CONNECT 0x01
#define MQTT_CONNECT_FLAGS 0x00

#define MQTT_VERSION_3_1_1 0x04

namespace iot::mqtt::server::packets {

    class Connect
        : public iot::mqtt::ControlPacketReceiver
        , public iot::mqtt::packets::Connect {
    public:
        explicit Connect(uint32_t remainingLength, uint8_t flags); // Server

    private:
        std::size_t deserializeVP(iot::mqtt::SocketContext* socketContext) override; // Server
        void propagateEvent(iot::mqtt::SocketContext* socketcontext) override;       // Server

    private:
        int state = 0;
    };

} // namespace iot::mqtt::server::packets

#endif // IOT_MQTT_SERVER_PACKETSNEW_CONNECT_H
