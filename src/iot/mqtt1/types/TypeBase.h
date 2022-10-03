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

#ifndef IOT_MQTT1_TYPES_TYPEBASE_H
#define IOT_MQTT1_TYPES_TYPEBASE_H

#include "core/socket/SocketContext.h" // IWYU pragma: export

namespace core::socket {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <vector> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    template <typename ValueTypeT>
    class TypeBase {
    protected:
        using ValueType = ValueTypeT;

    public:
        TypeBase(std::size_t size = sizeof(ValueType));

        virtual ~TypeBase() = default;

        virtual std::size_t construct(core::socket::SocketContext* socketContext);

        void setSize(std::size_t size);

        virtual void setValue(const ValueType& value) = 0;
        virtual ValueType getValue() const = 0;

        virtual std::vector<char> getValueAsVector() const;

        bool isComplete() const;
        bool isError() const;

        virtual void reset(std::size_t size = sizeof(ValueType));

    protected:
        core::socket::SocketContext* socketContext;

        std::vector<char> value;

        std::size_t length;
        std::size_t needed;

        bool complete = false;
        bool error = false;

        int state = 0;
    };

} // namespace iot::mqtt1::types

#endif // IOT_MQTT1_TYPES_TYPEBASE_H
