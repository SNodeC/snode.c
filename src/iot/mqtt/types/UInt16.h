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

#ifndef IOT_MQTT_TYPES_UINT16_H
#define IOT_MQTT_TYPES_UINT16_H

#include "iot/mqtt/types/TypeBase.h"

// IWYU pragma: no_include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    class UInt16 : public TypeBase<uint16_t> {
    public:
        UInt16() = default;
        UInt16(const UInt16&) = default;
        UInt16(UInt16&&) noexcept = default;

        UInt16& operator=(const UInt16&) = default;
        UInt16& operator=(UInt16&&) noexcept = default;

        ~UInt16() override;

        UInt16& operator=(const uint16_t& newValue);
        operator uint16_t() const;
    };

    extern template class TypeBase<uint16_t>;

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_UINT16_H
