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

#include "iot/mqtt1/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    ControlPacketFactory::ControlPacketFactory() {
    }

    std::size_t ControlPacketFactory::construct(iot::mqtt1::SocketContext* socketContext) {
        std::size_t consumed = staticHeader.construct(socketContext);

        error = staticHeader.isError();
        complete = staticHeader.isComplete();

        return consumed;
    }

    bool ControlPacketFactory::isComplete() {
        return complete;
    }

    bool ControlPacketFactory::isError() {
        return error;
    }

    uint8_t ControlPacketFactory::getPacketType() {
        return staticHeader.getPacketType();
    }

    uint8_t ControlPacketFactory::getPacketFlags() {
        return staticHeader.getReserved();
    }

    uint32_t ControlPacketFactory::getRemainingLength() {
        return staticHeader.getRemainingLength();
    }

    void ControlPacketFactory::reset() {
        complete = false;
        error = false;

        staticHeader.reset();
    }

} // namespace iot::mqtt1
