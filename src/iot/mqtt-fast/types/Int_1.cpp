/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "iot/mqtt-fast/types/Int_1.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::types {

    Int_1::Int_1(core::socket::SocketContext* socketContext)
        : iot::mqtt_fast::types::TypeBase(socketContext) {
    }

    std::size_t Int_1::construct() {
        const std::size_t consumed = read(buffer + length - needed, needed);

        needed -= consumed;
        completed = needed == 0;

        return consumed;
    }

    uint8_t Int_1::getValue() {
        return *reinterpret_cast<uint8_t*>(buffer);
    }

    void Int_1::reset() {
        length = 1;
        needed = 1;

        iot::mqtt_fast::types::TypeBase::reset();
    }

} // namespace iot::mqtt_fast::types
