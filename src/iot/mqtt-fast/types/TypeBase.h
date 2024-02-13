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

#ifndef IOT_MQTTFAST_TYPES_TYPESBASE_H
#define IOT_MQTTFAST_TYPES_TYPESBASE_H

namespace core::socket {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::types {

    class TypeBase {
    public:
        TypeBase(core::socket::SocketContext* socketContext);

        TypeBase(const TypeBase&) = delete;
        TypeBase(TypeBase&&) = default;

        TypeBase& operator=(const TypeBase&) = delete;
        TypeBase& operator=(TypeBase&&) = delete;

        virtual ~TypeBase() = default;

        virtual std::size_t construct() = 0;

        bool isCompleted() const;
        bool isError() const;

    protected:
        std::size_t read(char* buf, std::size_t count);
        virtual void reset();

        core::socket::SocketContext* socketContext;

        bool completed = false;
        bool error = false;
    };

} // namespace iot::mqtt_fast::types

#endif // IOT_MQTTFAST_TYPES_TYPESBASE_H
