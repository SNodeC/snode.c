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

#include "iot/mqtt/types/String.h"

#include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    String::String()
        : TypeBase(0) {
    }

    std::size_t String::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = stringLength.deserialize(mqttContext);
                if (!stringLength.isComplete()) {
                    break;
                }

                setSize(stringLength);
                state++;
                [[fallthrough]];
            case 1:
                consumed += TypeBase::deserialize(mqttContext);
                break;
        }

        // MUST close
        // Check for UTF-16 (U+D800 - U+DFFF) surrogates
        // Check for U+0000 in string

        // MAY close
        // Check for U+0001 - U+001F
        // Check for U+007F - U+009F
        // Check for non-characters

        return consumed;
    }

    std::vector<char> String::serialize() const {
        std::vector<char> returnVector = stringLength.serialize();
        returnVector.insert(returnVector.end(), value.begin(), value.end());

        return returnVector;
    }

    String& String::operator=(const std::string& newValue) {
        value = std::vector<char>(newValue.begin(), newValue.end());
        stringLength = static_cast<uint16_t>(value.size());

        return *this;
    }

    String::operator std::string() const {
        return std::string(value.begin(), value.end());
    }

    bool String::operator==(const std::string& rhsValue) const {
        return static_cast<std::string>(*this) == rhsValue;
    }

    bool String::operator!=(const std::string& rhsValue) const {
        return static_cast<std::string>(*this) != rhsValue;
    }

    void String::reset([[maybe_unused]] std::size_t size) {
        stringLength.reset();
        TypeBase::reset(0);

        state = 0;
    }

    template class TypeBase<std::string>;

} // namespace iot::mqtt::types
