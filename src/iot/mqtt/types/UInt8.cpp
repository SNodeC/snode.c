/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "iot/mqtt/types/UInt8.h"

#include "iot/mqtt/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    UInt8::~UInt8() {
    }

    UInt8& UInt8::operator=(const uint8_t& newValue) {
        *reinterpret_cast<uint8_t*>(value.data()) = newValue;

        return *this;
    }

    UInt8::operator uint8_t() const {
        return *reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
    }

    template class TypeBase<uint8_t>;

} // namespace iot::mqtt::types
