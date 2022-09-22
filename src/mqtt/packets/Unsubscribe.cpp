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

#include "Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::packets {

    Unsubscribe::Unsubscribe(ControlPacketFactory& controlPacketFactory)
        : mqtt::ControlPacket(controlPacketFactory) {
        uint32_t pointer = 0;

        packetIdentifier = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
        pointer += 2;

        while (data.size() > pointer) {
            uint16_t strLen = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += 2;

            std::string name = std::string(data.data() + pointer, strLen);
            pointer += strLen;

            topics.push_back(name);
        }
    }

    Unsubscribe::~Unsubscribe() {
    }

    uint16_t Unsubscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<std::string>& Unsubscribe::getTopics() const {
        return topics;
    }

} // namespace mqtt::packets
