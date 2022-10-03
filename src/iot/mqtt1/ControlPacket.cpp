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

#include "iot/mqtt1/ControlPacket.h"

#include "iot/mqtt1/StaticHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    ControlPacket::ControlPacket(uint8_t type, uint8_t reserved, uint32_t remainingLength)
        : type(type)
        , reserved(reserved)
        , remainingLength(remainingLength) {
    }

    ControlPacket::~ControlPacket() {
        if (currentPacket != nullptr) {
            delete currentPacket;
        }
    }

    std::size_t ControlPacket::_construct(SocketContext* socketContext) {
        std::size_t currentConsumed = deserialize(socketContext);
        consumed += currentConsumed;

        return currentConsumed;
    }

    std::vector<char> ControlPacket::serialize() const {
        std::vector<char> packet = getPacket();

        iot::mqtt1::StaticHeader staticHeader(getType(), getReserved());
        staticHeader.setRemainingLength(static_cast<uint32_t>(packet.size()));

        std::vector<char> packetStaticHeader = staticHeader.getPacket();
        packetStaticHeader.insert(packetStaticHeader.end(), packet.begin(), packet.end());

        return packetStaticHeader;
    }

    uint8_t ControlPacket::getType() const {
        return type;
    }

    uint8_t ControlPacket::getReserved() const {
        return reserved;
    }

    uint32_t ControlPacket::getRemainingLength() const {
        return remainingLength;
    }

    bool ControlPacket::isComplete() const {
        return complete;
    }

    bool ControlPacket::isError() const {
        return error;
    }

    std::size_t ControlPacket::getConsumed() const {
        return consumed;
    }

} // namespace iot::mqtt1
