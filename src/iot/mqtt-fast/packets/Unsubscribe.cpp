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

#include "iot/mqtt-fast/packets/Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Unsubscribe::Unsubscribe(uint16_t packetIdentifier, const std::list<std::string>& topics)
        : iot::mqtt_fast::ControlPacket(MQTT_UNSUBSCRIBE, 0x02)
        , packetIdentifier(packetIdentifier)
        , topics(std::move(topics)) {
        // V-Header
        putInt16(this->packetIdentifier);

        // Payload
        for (std::string& topic : this->topics) {
            putString(topic);
        }
    }

    Unsubscribe::Unsubscribe(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        // V-Header
        packetIdentifier = getInt16();

        // Payload
        for (std::string name = getString(); !name.empty(); name = getString()) {
            topics.push_back(name);
        }

        if (!isError()) {
            error = topics.empty();
        }
    }

    uint16_t Unsubscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<std::string>& Unsubscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt_fast::packets
