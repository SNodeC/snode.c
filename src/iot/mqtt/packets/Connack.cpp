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

#include "iot/mqtt/packets/Connack.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connack::Connack(uint8_t returncode, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK, MQTT_CONNACK_FLAGS) {
        this->returnCode = returncode;
        this->flags = flags;
    }

    Connack::Connack(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK, flags)
        , iot::mqtt::ControlPacketReceiver(remainingLength, MQTT_CONNACK_FLAGS) {
    }

    uint8_t Connack::getFlags() const {
        return flags;
    }

    uint8_t Connack::getReturnCode() const {
        return returnCode;
    }

    bool Connack::getSessionPresent() const {
        return sessionPresent;
    }

    std::vector<char> Connack::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = flags.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = returnCode.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    std::size_t Connack::deserializeVP(SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += flags.deserialize(socketContext);
                if (!flags.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += returnCode.deserialize(socketContext);

                if (!returnCode.isComplete()) {
                    break;
                }

                sessionPresent = returnCode & 0x01;

                complete = true;
                break;

                // no Payload
        }

        return consumed;
    }

    void Connack::propagateEvent(SocketContext* socketContext) {
        socketContext->_onConnack(*this);
    }

} // namespace iot::mqtt::packets
