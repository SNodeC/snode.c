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

#include "iot/mqtt/ControlPacket.h"

#include "iot/mqtt/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    ControlPacket::ControlPacket(uint8_t type, uint8_t reserved)
        : type(type)
        , reserved(reserved) {
    }

    ControlPacket::ControlPacket(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : type(controlPacketFactory.getPacketType())
        , reserved(controlPacketFactory.getPacketFlags())
        , data(std::move(controlPacketFactory.getPacket())) {
    }

    uint8_t ControlPacket::getType() const {
        return type;
    }

    uint8_t ControlPacket::getReserved() const {
        return reserved;
    }

    uint64_t ControlPacket::getRemainingLength() const {
        return data.size();
    }

    std::vector<char>& ControlPacket::getPacket() {
        if (!constructed) {
            packet.push_back(static_cast<char>(type << 0x04 | (reserved & 0x0F)));

            uint64_t remainingLength = data.size();
            do {
                uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
                remainingLength /= 0x80;
                if (remainingLength > 0) {
                    encodedByte |= 0x80;
                }
                packet.push_back(static_cast<char>(encodedByte));
            } while (remainingLength > 0);

            packet.insert(packet.end(), data.begin(), data.end());

            constructed = true;
        }

        return packet;
    }

} // namespace iot::mqtt
