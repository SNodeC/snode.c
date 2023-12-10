/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef IOT_MQTT_TYPES_STRING_H
#define IOT_MQTT_TYPES_STRING_H

#include "iot/mqtt/types/TypeBase.h" // IWYU pragma: export
#include "iot/mqtt/types/UInt16.h"

// IWYU pragma: no_include "iot/mqtt/types/TypeBase.hpp"

namespace iot::mqtt {
    class MqttContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    class String : public TypeBase<std::string> {
    public:
        String();

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext) override;
        std::vector<char> serialize() const override;

        String& operator=(const std::string& newValue) override;
        operator std::string() const override;

        bool operator==(const std::string& rhsValue) const;
        bool operator!=(const std::string& rhsValue) const;

        void reset(std::size_t size = sizeof(ValueType)) override;

    private:
        UInt16 stringLength;
    };

    extern template class TypeBase<std::string>;

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_STRING_H
