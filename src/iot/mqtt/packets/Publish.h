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

#ifndef IOT_MQTT_PACKETS_PUBLISH_H
#define IOT_MQTT_PACKETS_PUBLISH_H

#include "iot/mqtt/ControlPacketDeserializer.h" // IWYU pragma: export
#include "iot/mqtt/types/String.h"              // IWYU pragma: export
#include "iot/mqtt/types/StringRaw.h"           // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h"              // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    class Publish
        : public iot::mqtt::ControlPacketDeserializer
        , public iot::mqtt::ControlPacket {
    public:
        Publish(uint16_t packetIdentifier,
                const std::string& topic,
                const std::string& message,
                bool dup = false,
                uint8_t qoSLevel = 0,
                bool retain = false);
        explicit Publish(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) override;

    public:
        bool getDup() const;
        uint8_t getQoSLevel() const;
        uint16_t getPacketIdentifier() const;
        std::string getTopic() const;
        std::string getMessage() const;

        bool getRetain() const;

    private:
        iot::mqtt::types::UInt16 packetIdentifier;
        iot::mqtt::types::String topic;
        iot::mqtt::types::StringRaw message;

        bool dup = false;
        uint8_t qoSLevel = 0;
        bool retain = false;

        int state = 0;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_PUBLISH_H
