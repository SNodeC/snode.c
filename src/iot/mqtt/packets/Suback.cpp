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

#include "iot/mqtt/packets/Suback.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Suback::Suback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes)
        : iot::mqtt::ControlPacket(MQTT_SUBACK, MQTT_SUBACK_FLAGS) {
        this->packetIdentifier = packetIdentifier;
        this->returnCodes = returnCodes;
    }

    Suback::Suback(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt::ControlPacket(MQTT_SUBACK, reserved, remainingLength, MQTT_SUBACK_FLAGS) {
    }

    uint16_t Suback::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<uint8_t>& Suback::getReturnCodes() const {
        return returnCodes;
    }

    std::vector<char> Suback::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = packetIdentifier.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        for (uint8_t returnCode : returnCodes) {
            packet.push_back(static_cast<char>(returnCode));
        }

        return packet;
    }

    std::size_t Suback::deserializeVP([[maybe_unused]] SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += packetIdentifier.deserialize(socketContext);

                if (!packetIdentifier.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1: // Payload
                consumed += returnCode.deserialize(socketContext);

                if (!returnCode.isComplete()) {
                    break;
                } else {
                    returnCodes.push_back(returnCode);
                    returnCode.reset();

                    if (getConsumed() + consumed < this->getRemainingLength()) {
                        break;
                    }
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Suback::propagateEvent([[maybe_unused]] SocketContext* socketContext) {
        socketContext->_onSuback(*this);
    }

} // namespace iot::mqtt::packets
