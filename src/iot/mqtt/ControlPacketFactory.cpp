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

#include "iot/mqtt/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    ControlPacketFactory::ControlPacketFactory(iot::mqtt::SocketContext* socketContext)
        : socketContext(socketContext)
        , typeFlags(socketContext)
        , data(socketContext) {
    }

    std::size_t ControlPacketFactory::construct() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = typeFlags.construct();
                if (typeFlags.isCompleted()) {
                    state++;
                } else {
                    error = typeFlags.isError();
                }
                break;
            case 1:
                consumed = data.construct();
                if (data.isCompleted()) {
                    state++;
                    completed = true;
                } else {
                    error = data.isError();
                }
                break;
        }

        return consumed;
    }

    bool ControlPacketFactory::isComplete() {
        return completed;
    }

    bool ControlPacketFactory::isError() {
        return error;
    }

    std::vector<char>& ControlPacketFactory::getPacket() {
        return data.getValue();
    }

    uint8_t ControlPacketFactory::getPacketType() {
        return static_cast<uint8_t>(typeFlags.getValue() >> 0x04);
    }

    uint8_t ControlPacketFactory::getPacketFlags() {
        return static_cast<uint8_t>(typeFlags.getValue() & 0x0F);
    }

    uint64_t ControlPacketFactory::getRemainingLength() {
        return data.getValue().size();
    }

    void ControlPacketFactory::reset() {
        completed = false;
        error = false;
        state = 0;

        typeFlags.reset();
        data.reset();
    }

} // namespace iot::mqtt
