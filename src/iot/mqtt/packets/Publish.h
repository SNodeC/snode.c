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

#include "iot/mqtt/ControlPacket.h"   // IWYU pragma: export
#include "iot/mqtt/types/String.h"    // IWYU pragma: export
#include "iot/mqtt/types/StringRaw.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h"    // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    class Publish : public iot::mqtt::ControlPacket {
    public:
        Publish();
        Publish(uint16_t packetIdentifier,
                const std::string& topic,
                const std::string& message,
                bool dup = false,
                uint8_t qoSLevel = 0,
                bool retain = false);

    private:
        std::vector<char> serializeVP() const override;

    public:
        bool getDup() const;
        uint8_t getQoSLevel() const;
        uint16_t getPacketIdentifier() const;
        std::string getTopic() const;
        std::string getMessage() const;

        bool getRetain() const;

    protected:
        iot::mqtt::types::UInt16 packetIdentifier;
        iot::mqtt::types::String topic;
        iot::mqtt::types::StringRaw message;

        bool dup = false;
        uint8_t qoSLevel = 0;
        bool retain = false;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_PUBLISH_H
