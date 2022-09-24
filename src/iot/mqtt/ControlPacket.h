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

#ifndef IOT_MQTT_CONTROLPACKET_H
#define IOT_MQTT_CONTROLPACKET_H

#include "iot/mqtt/types/Binary.h"

namespace iot::mqtt {
    class ControlPacketFactory;
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class ControlPacket {
    public:
        explicit ControlPacket(uint8_t type, uint8_t reserved = 0);
        explicit ControlPacket(iot::mqtt::ControlPacketFactory& controlPacketFactory);

        uint8_t getType() const;
        uint8_t getReserved() const;
        uint64_t getRemainingLength() const;
        std::vector<char> getPacket();

        bool isError() const;

    private:
        uint8_t type;
        uint8_t reserved;

    protected:
        iot::mqtt::types::Binary data;
        bool error = false;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_CONTROLPACKET_H
