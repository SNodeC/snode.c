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

#include "iot/mqtt/packets/deserializer/Pingresp.h"

#include "iot/mqtt/client/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::deserializer {

    Pingresp::Pingresp(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacketDeserializer(remainingLength) {
        this->flags = flags;
    }

    std::size_t Pingresp::deserializeVP([[maybe_unused]] iot::mqtt::SocketContext* socketContext) {
        // no V-Header
        // no Payload

        complete = true;
        return 0;
    }

    void Pingresp::propagateEvent(iot::mqtt::SocketContext* socketContext) {
        dynamic_cast<iot::mqtt::client::SocketContext*>(socketContext)->_onPingresp(*this);
    }

} // namespace iot::mqtt::packets::deserializer
