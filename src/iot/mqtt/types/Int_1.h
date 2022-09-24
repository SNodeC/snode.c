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

#ifndef IOT_MQTT_TYPES_INT_1_H
#define IOT_MQTT_TYPES_INT_1_H

#include "iot/mqtt/types/TypeBase.h"

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    class Int_1 : public iot::mqtt::types::TypeBase {
    public:
        explicit Int_1(iot::mqtt::SocketContext* socketContext = nullptr);
        Int_1(const Int_1&) = default;

        Int_1& operator=(const Int_1&) = default;

        ~Int_1() override;

        std::size_t construct() override;

        uint8_t getValue();

        void reset() override;

    private:
        std::size_t needed = 1;
        std::size_t stillNeeded = 1;
        char buffer[1];
    };

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_INT_1_H
