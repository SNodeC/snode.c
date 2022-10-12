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

#include "iot/mqtt/server/packets/Pubrec.h"

#include "iot/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::deserializer {

    Pubrec::Pubrec(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacketDeserializer(remainingLength, flags, MQTT_PUBREC_FLAGS) {
        this->flags = flags;
    }

    std::size_t Pubrec::deserializeVP(SocketContext* socketContext) {
        // V-Header
        std::size_t consumed = packetIdentifier.deserialize(socketContext);
        complete = packetIdentifier.isComplete();

        // no Payload

        return consumed;
    }

    void Pubrec::propagateEvent(SocketContext* socketContext) {
        dynamic_cast<iot::mqtt::server::SocketContext*>(socketContext)->_onPubrec(*this);
    }

} // namespace iot::mqtt::packets::deserializer
