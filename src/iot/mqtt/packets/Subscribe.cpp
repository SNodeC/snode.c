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

#include "iot/mqtt/packets/Subscribe.h"

#include "iot/mqtt/types/Binary.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Subscribe::Subscribe(uint16_t packetIdentifier, const std::list<Topic>& topics)
        : iot::mqtt::ControlPacket(MQTT_SUBSCRIBE, 0x02)
        , packetIdentifier(packetIdentifier)
        , topics(std::move(topics)) {
        data.putInt16(this->packetIdentifier);

        for (const iot::mqtt::Topic& topic : this->topics) {
            data.putString(topic.getName());
            data.putInt8(topic.getRequestedQoS());
        }
    }

    Subscribe::Subscribe(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        packetIdentifier = data.getInt16();

        std::string name = "";
        do {
            name = data.getString();
            uint8_t requestedQoS = data.getInt8();

            if (name.length() > 0) {
                topics.push_back(iot::mqtt::Topic(name, requestedQoS));
            }
        } while (name.length() > 0);

        error = topics.empty();
    }

    uint16_t Subscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<iot::mqtt::Topic>& Subscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt::packets
