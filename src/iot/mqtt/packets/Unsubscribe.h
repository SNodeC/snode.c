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

#ifndef IOT_MQTT_PACKETS_UNSUBSCRIBE_H
#define IOT_MQTT_PACKETS_UNSUBSCRIBE_H

#include "iot/mqtt/types/String.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_UNSUBSCRIBE 0x0A
#define MQTT_UNSUBSCRIBE_FLAGS 0x02

namespace iot::mqtt::packets {

    class Unsubscribe {
    public:
        Unsubscribe() = default;

        uint16_t getPacketIdentifier() const;
        const std::list<std::string>& getTopics() const;

    protected:
        iot::mqtt::types::UInt16 packetIdentifier;
        iot::mqtt::types::String topic;

        std::list<std::string> topics;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_UNSUBSCRIBE_H
