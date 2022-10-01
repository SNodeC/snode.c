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

#ifndef IOT_MQTT_PACKETSNEW_PUBLISH_H
#define IOT_MQTT_PACKETSNEW_PUBLISH_H

#include "iot/mqtt/ControlPacket.h"

namespace iot::mqtt {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <string>  // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_PUBLISH 0x03

namespace iot::mqtt::packets {

    class Publish : public iot::mqtt::ControlPacket {
    public:
        Publish(uint16_t packetIdentifier,
                const std::string& topic,
                const std::string& message,
                bool dup = false,
                uint8_t qoSLevel = 0,
                bool retain = false);
        explicit Publish(iot::mqtt::ControlPacketFactory& controlPacketFactory);

        bool getDup() const;
        uint8_t getQoSLevel() const;
        uint16_t getPacketIdentifier() const;
        const std::string& getTopic() const;
        const std::string& getMessage() const;

        bool getRetain() const;

    private:
        uint16_t packetIdentifier = 0;
        std::string topic;
        std::string message;
        bool dup = false;
        uint8_t qoSLevel = 0;
        bool retain = false;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETSNEW_PUBLISH_H
