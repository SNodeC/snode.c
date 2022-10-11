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

#include "iot/mqtt/packets/serializer/Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::serializer {

    Unsubscribe::Unsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics)
        : iot::mqtt::ControlPacket(MQTT_UNSUBSCRIBE, MQTT_UNSUBSCRIBE_FLAGS) {
        this->packetIdentifier = packetIdentifier;
        this->topics = topics;
    }

    std::vector<char> Unsubscribe::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = packetIdentifier.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        for (const std::string& topic : topics) {
            packet.insert(packet.end(), topic.begin(), topic.end());
        }

        return packet;
    }

} // namespace iot::mqtt::packets::serializer
