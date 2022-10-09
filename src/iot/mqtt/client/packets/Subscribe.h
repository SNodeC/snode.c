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

#ifndef IOT_MQTT_CLIENT_PACKETSNEW_SUBSCRIBE_H
#define IOT_MQTT_CLIENT_PACKETSNEW_SUBSCRIBE_H

#include "iot/mqtt/ControlPacketSender.h"
#include "iot/mqtt/packets/Subscribe.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client::packets {

    class Subscribe
        : public iot::mqtt::ControlPacketSender
        , public iot::mqtt::packets::Subscribe {
    public:
        Subscribe() = default;
        Subscribe(uint16_t packetIdentifier, std::list<iot::mqtt::Topic>& topics); // Client

    private:
        std::vector<char> serializeVP() const override; // Client
    };

} // namespace iot::mqtt::client::packets

#endif // IOT_MQTT_CLIENT_PACKETSNEW_SUBSCRIBE_H
