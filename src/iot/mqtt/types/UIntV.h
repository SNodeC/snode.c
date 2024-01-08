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

#ifndef IOT_MQTT_TYPES_UINTV_H
#define IOT_MQTT_TYPES_UINTV_H

#include "iot/mqtt/types/TypeBase.h" // IWYU pragma: export

// IWYU pragma: no_include "iot/mqtt/types/TypeBase.hpp"

namespace iot::mqtt {
    class MqttContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    class UIntV : public TypeBase<uint32_t> {
    public:
        UIntV();
        UIntV(const UIntV&) = default;
        UIntV(UIntV&&) noexcept = default;

        ~UIntV() override;

        UIntV& operator=(const UIntV&) = default;
        UIntV& operator=(UIntV&&) noexcept = default;

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext) override;

        UIntV& operator=(const uint32_t& newValue);
        operator uint32_t() const;

        bool isError() const;

        void reset(std::size_t size = sizeof(ValueType)) override;

    private:
        bool error = false;
    };

    extern template class TypeBase<uint32_t>;

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_UINTV_H
