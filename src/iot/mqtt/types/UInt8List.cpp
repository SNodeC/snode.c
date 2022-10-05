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

#include "iot/mqtt/types/UInt8List.h"

#include "iot/mqtt/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    std::list<uint8_t> UInt8List::operator=(const std::list<uint8_t>& newValue) {
        value = std::vector<char>(newValue.begin(), newValue.end());
        return *this;
    }

    UInt8List::operator std::list<uint8_t>() const {
        return std::list<uint8_t>(value.begin(), value.end());
    }

    bool UInt8List::operator==(const std::list<uint8_t>& rhsValue) const {
        return static_cast<std::list<uint8_t>>(*this) == rhsValue;
    }

    bool UInt8List::operator!=(const std::list<uint8_t>& rhsValue) const {
        return static_cast<std::list<uint8_t>>(*this) != rhsValue;
    }

    template class TypeBase<std::list<uint8_t>>;

} // namespace iot::mqtt::types
