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

#include "iot/mqtt/types/BinaryData.h"

#include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    BinaryData::BinaryData()
        : TypeBase(0) {
    }

    BinaryData::~BinaryData() {
    }

    std::size_t BinaryData::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = valueLength.deserialize(mqttContext);
                if (!valueLength.isComplete()) {
                    break;
                }

                setSize(valueLength);
                state++;
                [[fallthrough]];
            case 1:
                consumed += TypeBase::deserialize(mqttContext);
                break;
        }

        return consumed;
    }

    std::vector<char> BinaryData::serialize() const {
        std::vector<char> returnVector = valueLength.serialize();
        returnVector.insert(returnVector.end(), value.begin(), value.end());

        return returnVector;
    }

    BinaryData& BinaryData::operator=(const std::vector<char>& newValue) {
        value = newValue;
        valueLength = static_cast<uint16_t>(value.size());

        return *this;
    }

    BinaryData::operator std::vector<char>() const {
        return value;
    }

    bool BinaryData::operator==(const std::vector<char>& rhsValue) const {
        return static_cast<std::vector<char>>(*this) == rhsValue;
    }

    bool BinaryData::operator!=(const std::vector<char>& rhsValue) const {
        return static_cast<std::vector<char>>(*this) != rhsValue;
    }

    void BinaryData::reset([[maybe_unused]] std::size_t size) {
        valueLength.reset();
        TypeBase::reset(0);

        state = 0;
    }

    template class TypeBase<std::vector<char>>;

} // namespace iot::mqtt::types
