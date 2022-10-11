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

#ifndef IOT_MQTT_CLIENT_PACKETS_PINGREQ_H
#define IOT_MQTT_CLIENT_PACKETS_PINGREQ_H

#include "iot/mqtt/ControlPacketSender.h"
#include "iot/mqtt/packets/Pingreq.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::serializer {

    class Pingreq
        : public iot::mqtt::ControlPacketSender
        , public iot::mqtt::packets::Pingreq {
    public:
        explicit Pingreq();

    private:
        std::vector<char> serializeVP() const override;
    };

} // namespace iot::mqtt::packets::serializer

#endif // IOT_MQTT_CLIENT_PACKETS_PINGREQ_H
