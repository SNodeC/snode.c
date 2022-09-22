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

#include "iot/mqtt/ControlPacket.h"

namespace iot::mqtt {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <string>  // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    class Publish : public iot::mqtt::ControlPacket {
    public:
        explicit Publish(iot::mqtt::ControlPacketFactory& controlPacketFactory);
        Publish(const Publish&) = default;

        Publish& operator=(const Publish&) = default;

        ~Publish();

        bool getDup() const;
        uint8_t getQoSLevel() const;
        uint16_t getPacketIdentifier() const;
        const std::string& getName() const;
        const std::string& getMessage() const;

    private:
        bool dup = false;
        uint8_t qoSLevel = 0;
        bool retain = false;
        uint16_t packetIdentifier = 0;
        std::string name;
        std::string message;
    };

} // namespace iot::mqtt::packets

#endif // IOT_MQTT_PACKETS_PUBLISH_H
