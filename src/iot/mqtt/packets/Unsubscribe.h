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

#include "iot/mqtt/ControlPacket.h"

namespace iot::mqtt {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <list>    // IWYU pragma: export
#include <string>  // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_UNSUBSCRIBE 0x0A

namespace iot::mqtt::packets {

    class Unsubscribe : public iot::mqtt::ControlPacket {
    public:
        Unsubscribe(uint16_t packetIdentifier, const std::list<std::string>& topics);
        explicit Unsubscribe(iot::mqtt::ControlPacketFactory& controlPacketFactory);

        uint16_t getPacketIdentifier() const;

        const std::list<std::string>& getTopics() const;

    private:
        uint16_t packetIdentifier;
        std::list<std::string> topics;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_UNSUBSCRIBE_H
