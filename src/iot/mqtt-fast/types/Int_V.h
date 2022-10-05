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

#ifndef IOT_MQTTFAST_TYPES_INT_V_H
#define IOT_MQTTFAST_TYPES_INT_V_H

#include "iot/mqtt-fast/types/TypeBase.h"

namespace core::socket {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt_fast::types {

    class Int_V : public iot::mqtt_fast::types::TypeBase {
    public:
        explicit Int_V(core::socket::SocketContext* socketContext = nullptr);

        std::size_t construct() override;

        uint32_t getValue();

        void reset() override;

    private:
        std::size_t multiplier = 1;
        std::uint32_t value = 0;
    };

} // namespace iot::mqtt_fast::types

#endif // IOT_MQTTFAST_TYPES_INT_V_H
