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

#ifndef IOT_MQTT_TYPES_INT_4_H
#define IOT_MQTT_TYPES_INT_4_H

#include "iot/mqtt/types/TypeBase.h"

namespace iot::mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    class Int_4 : public iot::mqtt::types::TypeBase {
    public:
        explicit Int_4(iot::mqtt::SocketContext* socketContext = nullptr);
        Int_4(const Int_4&) = default;

        Int_4& operator=(const Int_4&) = default;

        ~Int_4() override;

        std::size_t construct() override;

        uint32_t getValue();

        void reset() override;

    private:
        std::size_t needed = 4;
        std::size_t stillNeeded = 4;
        char buffer[4];
    };

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_INT_4_H
