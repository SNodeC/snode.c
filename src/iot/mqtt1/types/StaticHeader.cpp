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

#include "iot/mqtt1/types/StaticHeader.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    StaticHeader::StaticHeader() {
    }

    StaticHeader::StaticHeader(uint8_t packetType, uint8_t reserved, uint32_t remainingLength) {
        _typeReserved.setValue(packetType << 4 & reserved & 0x0F);
        _remainingLength.setValue(remainingLength);
    }

    StaticHeader::~StaticHeader() {
    }

    std::size_t StaticHeader::construct(iot::mqtt1::SocketContext* socketContext) {
        std::size_t consumedTotal = 0;
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = _typeReserved.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _typeReserved.isError())) {
                    break;
                } else if (_typeReserved.isComplete()) {
                    state++;
                }
                [[fallthrough]];
            case 1:
                consumed = _remainingLength.construct(socketContext);
                consumedTotal += consumed;

                complete = _remainingLength.isComplete();
                error = _remainingLength.isError();
                break;
            default:
                break;
        }

        return consumedTotal;
    }

    uint8_t StaticHeader::getPacketType() const {
        return static_cast<uint8_t>(_typeReserved.getValue() >> 0x04);
    }

    uint8_t StaticHeader::getReserved() const {
        return static_cast<uint8_t>(_typeReserved.getValue() & 0x0F);
    }

    void StaticHeader::setRemainingLength(uint32_t remainingLength) {
        _remainingLength.setValue(remainingLength);
    }

    uint32_t StaticHeader::getRemainingLength() const {
        return _remainingLength.getValue();
    }

    bool StaticHeader::isComplete() const {
        return complete;
    }

    bool StaticHeader::isError() const {
        return error;
    }

    std::vector<char> StaticHeader::getPacket() {
        std::vector<char> packet;

        packet.insert(packet.end(), _typeReserved.getValueAsVector().begin(), _typeReserved.getValueAsVector().end());
        packet.insert(packet.begin(), _remainingLength.getValueAsVector().begin(), _remainingLength.getValueAsVector().end());

        return packet;
    }

} // namespace iot::mqtt1::types
