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

#include "iot/mqtt1/packets/Pingreq.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::packets {

    Pingreq::Pingreq()
        : iot::mqtt1::ControlPacket(MQTT_PINGREQ, 0, 0) {
    }

    Pingreq::Pingreq(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt1::ControlPacket(MQTT_PINGREQ, reserved, remainingLength) {
    }

    std::vector<char> Pingreq::getPacket() const {
        return std::vector<char>();
    }

    std::size_t Pingreq::deserialize([[maybe_unused]] SocketContext* socketContext) {
        complete = true;
        return 0;
    }

    void Pingreq::propagateEvent([[maybe_unused]] SocketContext* socketContext) const {
        socketContext->_onPingreq(*this);
    }

} // namespace iot::mqtt1::packets
