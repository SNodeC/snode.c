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

#ifndef IOT_MQTT_TYPES_STRINGRAW_H
#define IOT_MQTT_TYPES_STRINGRAW_H

#include "iot/mqtt/types/TypeBase.h" // IWYU pragma: export

// IWYU pragma: no_include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    class StringRaw : public TypeBase<std::string> {
    public:
        StringRaw();

        std::string operator=(const std::string& newValue) override;
        operator std::string() const override;

        bool operator==(const std::string& rhsValue) const override;
        bool operator!=(const std::string& rhsValue) const override;
    };

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_STRINGRAW_H
