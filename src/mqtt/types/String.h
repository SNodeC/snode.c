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

#ifndef MQTT_TYPES_STRING_H
#define MQTT_TYPES_STRING_H

#include "mqtt/types/TypesBase.h"

namespace mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::types {

    class String : public mqtt::types::TypesBase {
    public:
        explicit String(mqtt::SocketContext* socketContext);
        String(const String&) = default;

        String& operator=(const String&) = default;

        ~String() override;

        std::size_t construct() override;
    };

} // namespace mqtt::types

#endif // MQTT_TYPES_STRING_H
