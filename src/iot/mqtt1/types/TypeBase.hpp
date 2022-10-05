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

#include "core/socket/SocketContext.h"
#include "iot/mqtt1/types/TypeBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    template <typename ValueType>
    TypeBase<ValueType>::TypeBase(std::size_t size) {
        value.resize(size);
        length = size;
        needed = size;
    }

    template <typename ValueType>
    std::size_t TypeBase<ValueType>::deserialize(core::socket::SocketContext* socketContext) {
        std::size_t consumed = 0;

        consumed = socketContext->readFromPeer(value.data() + length - needed, static_cast<std::size_t>(needed));
        needed -= consumed;
        complete = needed == 0;

        return consumed;
    }

    template <typename ValueType>
    void TypeBase<ValueType>::setSize(std::size_t size) {
        value.resize(size);

        length = size;
        needed = size;
    }

    template <typename ValueType>
    std::vector<char> iot::mqtt1::types::TypeBase<ValueType>::serialize() const {
        return value;
    }

    template <typename ValueType>
    bool TypeBase<ValueType>::isComplete() const {
        return complete;
    }

    template <typename ValueType>
    bool TypeBase<ValueType>::isError() const {
        return error;
    }

    template <typename ValueType>
    void TypeBase<ValueType>::reset(std::size_t size) {
        value.resize(size);

        length = size;
        needed = size;

        error = false;
        complete = false;

        state = 0;
    }

} // namespace iot::mqtt1::types
