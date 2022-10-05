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

#include "iot/mqtt1/types/UInt64.h"

#include "iot/mqtt1/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    uint64_t UInt64::operator=(const uint64_t& newValue) {
        return *reinterpret_cast<uint64_t*>(value.data()) = htobe64(newValue);
    }

    UInt64::operator uint64_t() const {
        return be64toh(*reinterpret_cast<uint64_t*>(const_cast<char*>(value.data())));
    }

    bool UInt64::operator==(const uint64_t& rhsValue) const {
        return static_cast<uint64_t>(*this) == rhsValue;
    }

    bool UInt64::operator!=(const uint64_t& rhsValue) const {
        return static_cast<uint64_t>(*this) != rhsValue;
    }

    template class TypeBase<uint64_t>;

} // namespace iot::mqtt1::types
