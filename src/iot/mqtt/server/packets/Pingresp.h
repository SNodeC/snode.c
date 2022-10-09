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

#ifndef IOT_MQTT_SERVER_PACKETSNEW_PINGRESP_H
#define IOT_MQTT_SERVER_PACKETSNEW_PINGRESP_H

#include "iot/mqtt/ControlPacketSender.h" // IWYU pragma: export
#include "iot/mqtt/packets/Pingresp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_PINGRESP 0x0D
#define MQTT_PINGRESP_FLAGS 0x00

namespace iot::mqtt::server::packets {

    class Pingresp
        : public iot::mqtt::ControlPacketSender
        , public iot::mqtt::packets::Pingresp {
    public:
        explicit Pingresp(); // Server

    private:
        std::vector<char> serializeVP() const override; // Server
    };

} // namespace iot::mqtt::server::packets

#endif // IOT_MQTT_PACKETSNEW_PINGRESP_H
