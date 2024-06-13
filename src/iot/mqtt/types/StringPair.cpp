/*
 * snode.c - a slim toolkit for network communication
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

#include "StringPair.h"

#include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    StringPair::StringPair() {
    }

    StringPair::StringPair(const std::string& name, const std::string& value)
        : name(name)
        , value(value) {
    }

    StringPair::~StringPair() {
    }

    std::size_t StringPair::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed += name.deserialize(mqttContext);
                if (!name.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += value.deserialize(mqttContext);

                complete = value.isComplete();
                break;
        }

        return consumed;
    }

    std::vector<char> StringPair::serialize() const {
        std::vector<char> returnVector = name.serialize();
        std::vector<char> valueSerialization = value.serialize();

        returnVector.insert(returnVector.end(), valueSerialization.begin(), valueSerialization.end());

        return returnVector;
    }

    void StringPair::reset(std::size_t size) {
        name.reset(size);
        value.reset(size);
    }

} // namespace iot::mqtt::types
