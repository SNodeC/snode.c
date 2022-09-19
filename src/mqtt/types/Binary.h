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

#ifndef MQTT_TYPES_BINARY_H
#define MQTT_TYPES_BINARY_H

#include "mqtt/types/Int_V.h"
#include "mqtt/types/TypesBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::types {

    class Binary : public mqtt::types::TypesBase {
    public:
        explicit Binary(mqtt::SocketContext* socketContext);
        Binary(const Binary&) = default;

        Binary& operator=(const Binary&) = default;

        ~Binary() override;

        std::size_t construct() override;

        std::vector<char> getValue();

    private:
        uint8_t state = 0;
        std::size_t needed = 0;
        std::size_t stillNeeded = 0;

        mqtt::types::Int_V length;
        std::vector<char> data;
    };

} // namespace mqtt::types

#endif // MQTT_TYPES_BINARY_H
