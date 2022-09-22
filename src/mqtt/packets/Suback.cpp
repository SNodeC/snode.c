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

#include "Suback.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::packets {

    Suback::Suback(ControlPacketFactory& controlPacketFactory)
        : mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        for (; pointer < this->getRemainingLength(); ++pointer) {
            returnCodes.push_back(static_cast<uint8_t>(*(data.data() + pointer)));
        }
    }

    Suback::~Suback() {
    }

    uint16_t Suback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<uint8_t>& Suback::getReturnCodes() const {
        return returnCodes;
    }

} // namespace mqtt::packets
