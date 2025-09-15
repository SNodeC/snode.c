/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "iot/mqtt/types/UIntV.h"

#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    UIntV::UIntV()
        : TypeBase(0) {
    }

    UIntV::~UIntV() {
    }

    std::size_t UIntV::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        do {
            char byte = 0;
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

    UIntV& UIntV::operator=(const uint32_t& newValue) {
        uint32_t remainingValue = newValue;
        value.clear();

        do {
            char encodedByte = static_cast<char>(remainingValue % 0x80);

            remainingValue /= 0x80;
            if (remainingValue > 0) {
                encodedByte |= static_cast<char>(0x80);
            }

            value.push_back(encodedByte);
        } while (remainingValue > 0);

        return *this;
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
