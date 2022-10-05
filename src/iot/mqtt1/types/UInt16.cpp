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

#include "iot/mqtt1/types/UInt16.h"

#include "iot/mqtt1/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    uint16_t UInt16::operator=(const uint16_t& newValue) {
        return *reinterpret_cast<uint16_t*>(value.data()) = htobe16(newValue);
    }

    UInt16::operator uint16_t() const {
        return be16toh(*reinterpret_cast<uint16_t*>(const_cast<char*>(value.data())));
    }

    bool UInt16::operator==(const uint16_t& rhsValue) const {
        return static_cast<uint16_t>(*this) == rhsValue;
    }

    bool UInt16::operator!=(const uint16_t& rhsValue) const {
        return static_cast<uint16_t>(*this) != rhsValue;
    }

    template class TypeBase<uint16_t>;

} // namespace iot::mqtt1::types
