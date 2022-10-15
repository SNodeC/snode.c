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

#include "iot/mqtt/StaticHeader.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    StaticHeader::StaticHeader() {
    }

    StaticHeader::StaticHeader(uint8_t type, uint8_t flags, uint32_t remainingLength) {
        this->typeFlags = static_cast<uint8_t>((type << 4) | (flags & 0x0F));
        this->remainingLength = remainingLength;
    }

    StaticHeader::~StaticHeader() {
    }

    std::size_t StaticHeader::deserialize(iot::mqtt::SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed += typeFlags.deserialize(socketContext);

                if (!typeFlags.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 1:
                consumed += remainingLength.deserialize(socketContext);

                complete = remainingLength.isComplete();
                error = remainingLength.isError();

                break;
        }

        return consumed;
    }

    uint8_t StaticHeader::getPacketType() const {
        return static_cast<uint8_t>(typeFlags >> 0x04);
    }

    uint8_t StaticHeader::getFlags() const {
        return static_cast<uint8_t>(typeFlags & 0x0F);
    }

    void StaticHeader::setRemainingLength(uint32_t remainingLength) {
        this->remainingLength = remainingLength;
    }

    uint32_t StaticHeader::getRemainingLength() const {
        return remainingLength;
    }

    bool StaticHeader::isComplete() const {
        return complete;
    }

    bool StaticHeader::isError() const {
        return error;
    }

    std::vector<char> StaticHeader::serialize() {
        std::vector<char> packet = typeFlags.serialize();

        std::vector<char> tmpVector = remainingLength.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    void StaticHeader::reset() {
        typeFlags.reset();
        remainingLength.reset();

        complete = false;
        error = false;

        state = 0;
    }

} // namespace iot::mqtt
