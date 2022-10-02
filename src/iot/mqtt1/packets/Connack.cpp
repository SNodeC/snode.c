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

    Connack::Connack(uint8_t reason, uint8_t flags)
        : iot::mqtt1::ControlPacket(MQTT_CONNACK) {
        // V-Header
        _flags.setValue(flags);
        _reason.setValue(reason);

        // no Payload
    }

    uint8_t Connack::getFlags() const {
        return _flags.getValue();
    }

    uint8_t Connack::getReason() const {
        return _reason.getValue();
    }

    std::vector<char> Connack::getPacket() const {
        std::vector<char> packet;

        packet.insert(packet.end(), _flags.getValueAsVector().begin(), _flags.getValueAsVector().end());
        packet.insert(packet.end(), _reason.getValueAsVector().begin(), _reason.getValueAsVector().end());

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

                if (consumed == 0 || (error = _flags.isError())) {
                    break;
                } else if (_flags.isComplete()) {
                    state++;
                }
                [[fallthrough]];
            case 1:
                consumed = _reason.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _reason.isError())) {
                    break;
                } else if (_reason.isComplete()) {
                    state++;
                }
                [[fallthrough]];

            default:
                socketContext->_onConnack(*this);
                complete = true;
                break;
        }

        return consumedTotal;
    }

} // namespace iot::mqtt1::packets
