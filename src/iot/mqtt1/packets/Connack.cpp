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

#include "iot/mqtt1/packets/Connack.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::packets {

    Connack::Connack(uint8_t returncode, uint8_t flags)
        : iot::mqtt1::ControlPacket(MQTT_CONNACK, 0, 0) {
        _returnCode.setValue(returncode);
        _flags.setValue(flags);
    }

    Connack::Connack(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt1::ControlPacket(MQTT_CONNACK, reserved, remainingLength) {
    }

    uint8_t Connack::getFlags() const {
        return _flags.getValue();
    }

    uint8_t Connack::getReturnCode() const {
        return _returnCode.getValue();
    }

    std::vector<char> Connack::getPacket() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = _flags.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = _returnCode.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    std::size_t Connack::construct(SocketContext* socketContext) {
        std::size_t consumedTotal = 0;
        std::size_t consumed = 0;

        switch (state) {
            // V-Header
            case 0:
                consumed = _flags.construct(socketContext);
                consumedTotal += consumed;

                if ((error = _flags.isError()) || !_flags.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 1:
                consumed = _returnCode.construct(socketContext);
                consumedTotal += consumed;

                if ((error = _returnCode.isError()) || !_returnCode.isComplete()) {
                    break;
                }
                [[fallthrough]];
            default:
                complete = true;
                break;
        }

        return consumedTotal;
    }

    void Connack::propagateEvent(SocketContext* socketContext) const {
        socketContext->_onConnack(*this);
    }

} // namespace iot::mqtt1::packets
