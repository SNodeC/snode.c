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

#include "iot/mqtt/types/Int_1.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    Int_1::Int_1(iot::mqtt::SocketContext* socketContext)
        : mqtt::types::TypeBase(socketContext) {
    }

    Int_1::~Int_1() {
    }

    std::size_t Int_1::construct() {
        std::size_t consumed = socketContext->readFromPeer(buffer + needed - stillNeeded, static_cast<std::size_t>(stillNeeded));

        stillNeeded -= consumed;
        completed = stillNeeded == 0;

        return consumed;
    }

    uint8_t Int_1::getValue() {
        return static_cast<uint8_t>(*buffer);
    }

    void Int_1::reset() {
        needed = 1;
        stillNeeded = 1;

        mqtt::types::TypeBase::reset();
    }

} // namespace iot::mqtt::types
