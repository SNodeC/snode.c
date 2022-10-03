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

#ifndef IOT_MQTT1_TYPES_UINTV_H
#define IOT_MQTT1_TYPES_UINTV_H

#include "iot/mqtt1/types/TypeBase.h"

// IWYU pragma: no_include "iot/mqtt1/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    class UIntV : public TypeBase<uint32_t> {
    public:
        UIntV();

        std::size_t construct(core::socket::SocketContext* socketContext) override;

        void setValue(const uint32_t& newValue) override;
        uint32_t getValue() const override;

        void reset(std::size_t size = sizeof(ValueType)) override;
    };

} // namespace iot::mqtt1::types

#endif // IOT_MQTT1_TYPES_UINTV_H
