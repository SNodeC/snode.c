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

#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/types/TypeBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    template <typename ValueType>
    TypeBase<ValueType>::TypeBase(std::size_t size) {
        value.resize(size);
        length = size;
        needed = size;
    }

    template <typename ValueType>
    std::size_t TypeBase<ValueType>::deserialize(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        consumed = mqttContext->recv(value.data() + length - needed, needed);
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

    template <typename ValueTypeT>
    std::size_t TypeBase<ValueTypeT>::size() {
        return length;
    }

    template <typename ValueType>
    std::vector<char> iot::mqtt::types::TypeBase<ValueType>::serialize() const {
        return value;
    }

    template <typename ValueType>
    bool TypeBase<ValueType>::isComplete() const {
        return complete;
    }

    template <typename ValueType>
    void TypeBase<ValueType>::reset(std::size_t size) {
        value.resize(size);

        length = size;
        needed = size;

        complete = false;

        state = 0;
    }

} // namespace iot::mqtt::types
