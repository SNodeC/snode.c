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
        : iot::mqtt::ControlPacket(MQTT_CONNACK, 0x00, 0) {
        this->returnCode = returncode;
        this->flags = flags;
    }

    Connack::Connack(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt::ControlPacket(MQTT_CONNACK, reserved, remainingLength) {
        error = reserved != 0x00;
    }

    uint8_t Connack::getFlags() const {
        return flags;
    }

    uint8_t Connack::getReturnCode() const {
        return returnCode;
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
            // V-Header
            case 0:
                consumed += flags.deserialize(socketContext);

                if ((error = flags.isError()) || !flags.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += returnCode.deserialize(socketContext);

                error = returnCode.isError();
                complete = returnCode.isComplete();

                if ((error = returnCode.isError() || !returnCode.isComplete())) {
                    break;
                } else if (returnCode != MQTT_CONNACK_ACCEPT) {
                    socketContext->shutdown();
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Connack::propagateEvent(SocketContext* socketContext) {
        socketContext->_onConnack(*this);
    }

} // namespace iot::mqtt::packets
