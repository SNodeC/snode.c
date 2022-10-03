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

#ifndef IOT_MQTT_PACKETSNEW_UNSUBSCRIBE_H
#define IOT_MQTT_PACKETSNEW_UNSUBSCRIBE_H

#include "iot/mqtt1/ControlPacket.h"
#include "iot/mqtt1/Topic.h" // IWYU pragma: export
#include "iot/mqtt1/types/String.h"
#include "iot/mqtt1/types/UInt16.h"

namespace iot::mqtt1 {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint> // IWYU pragma: export
#include <list>    // IWYU pragma: export
#include <string>  // IWYU pragma: export
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_UNSUBSCRIBE 0x0A

namespace iot::mqtt1::packets {

    class Unsubscribe : public iot::mqtt1::ControlPacket {
    public:
        Unsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics);
        explicit Unsubscribe(uint32_t remainingLength, uint8_t reserved);

    private:
        std::size_t deserialize(SocketContext* socketContext) override;
        void propagateEvent(SocketContext* socketContext) const override;

    public:
        uint16_t getPacketIdentifier() const;
        const std::list<std::string>& getTopics() const;

    private:
        std::vector<char> getPacket() const override;

        iot::mqtt1::types::UInt16 packetIdentifier;
        iot::mqtt1::types::String topic;

        std::list<std::string> topics;

        int state = 0;
    };

} // namespace iot::mqtt1::packets

#endif // IOT_MQTT_PACKETSNEW_UNSUBSCRIBE_H
