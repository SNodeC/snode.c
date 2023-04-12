/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/types/UIntV.h"

#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    UIntV::UIntV()
        : TypeBase(0) {
    }

    std::size_t UIntV::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        do {
            char byte;
            consumed = mqttContext->recv(&byte, 1);

            if (consumed > 0) {
                value.push_back(byte);

                if (value.size() > sizeof(uint32_t)) {
                    error = true;
                } else {
                    complete = (byte & 0x80) == 0;
                }
            }
        } while (consumed > 0 && !complete && !error);

        return consumed;
    }

    uint32_t UIntV::operator=(const uint32_t& newValue) {
        uint32_t remainingValue = newValue;
        value.resize(0);

        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingValue % 0x80);

            remainingValue /= 0x80;
            if (remainingValue > 0) {
                encodedByte |= 0x80;
            }

            value.push_back(static_cast<char>(encodedByte));
        } while (remainingValue > 0);

        return newValue;
    }

    UIntV::operator uint32_t() const {
        uint32_t uint32Value = 0;
        uint32_t multiplicator = 1;

        for (std::size_t i = 0; i < value.size(); i++, multiplicator *= 0x80) {
            uint32Value += static_cast<uint8_t>(value[i] & 0x7F) * multiplicator;
        }

        return uint32Value;
    }

    bool UIntV::isError() const {
        return error;
    }

    void UIntV::reset([[maybe_unused]] std::size_t size) {
        error = false;

        TypeBase::reset(0);
    }

} // namespace iot::mqtt::types
