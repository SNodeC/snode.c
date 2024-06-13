/*
 * snode.c - a slim toolkit for network communication
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

#ifndef IOT_MQTT_TYPES_STRINGPAIR_H
#define IOT_MQTT_TYPES_STRINGPAIR_H

#include "iot/mqtt/types/String.h"
#include "iot/mqtt/types/TypeBase.h"

namespace iot::mqtt {
    class MqttContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    class StringPair : public TypeBase<std::vector<char>> {
    public:
        StringPair();
        StringPair(const std::string& name, const std::string& value);

        StringPair(const StringPair&) = default;

        ~StringPair() override;

        StringPair& operator=(const StringPair&) = default;
        StringPair& operator=(StringPair&&) noexcept = default;

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext) override;
        std::vector<char> serialize() const override;

        StringPair& operator=(const std::string& newValue);
        operator std::string() const;

        bool operator==(const StringPair& rhsValue) const;
        bool operator!=(const StringPair& rhsValue) const;

        void reset(std::size_t size = 0) override;

    private:
        iot::mqtt::types::String name;
        iot::mqtt::types::String value;
    };

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_STRINGPAIR_H
