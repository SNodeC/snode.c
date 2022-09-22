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

#ifndef MQTT_PACKETS_SUBACK_H
#define MQTT_PACKETS_SUBACK_H

#include "mqtt/ControlPacket.h"

namespace mqtt {
    class ControlPacketFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <list>    // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::packets {

    class Suback : public mqtt::ControlPacket {
    public:
        explicit Suback(mqtt::ControlPacketFactory& controlPacketFactory);
        Suback(const Suback&) = default;

        Suback& operator=(const Suback&) = default;

        ~Suback();

        uint16_t getPacketIdentifier() const;

        const std::list<uint8_t>& getReturnCodes() const;

    private:
        uint16_t packetIdentifier;
        std::list<uint8_t> returnCodes;
    };

} // namespace mqtt::packets

#endif // MQTT_PACKETS_SUBACK_H
