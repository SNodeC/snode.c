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

#include "iot/mqtt/packets/Puback.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Puback::Puback(const uint16_t packetIdentifier)
        : iot::mqtt::ControlPacket(MQTT_PUBACK, MQTT_PUBACK_FLAGS) {
        this->packetIdentifier = packetIdentifier;
    }

    Puback::Puback(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK, flags)
        , iot::mqtt::ControlPacketReceiver(remainingLength, MQTT_PUBACK_FLAGS) {
    }

    uint16_t Puback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    std::vector<char> Puback::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = packetIdentifier.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    std::size_t Puback::deserializeVP(SocketContext* socketContext) {
        // V-Header
        std::size_t consumed = packetIdentifier.deserialize(socketContext);
        complete = packetIdentifier.isComplete();

        // no Payload

        return consumed;
    }

    void Puback::propagateEvent(SocketContext* socketContext) {
        socketContext->_onPuback(*this);
    }

} // namespace iot::mqtt::packets
