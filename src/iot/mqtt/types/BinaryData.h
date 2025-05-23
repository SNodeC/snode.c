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

#ifndef IOT_MQTT_TYPES_BINARYDATA_H
#define IOT_MQTT_TYPES_BINARYDATA_H

#include "iot/mqtt/types/TypeBase.h"
#include "iot/mqtt/types/UInt16.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::types {

    class BinaryData : public TypeBase<std::vector<char>> {
    public:
        BinaryData();
        BinaryData(const BinaryData&) = default;
        BinaryData(BinaryData&&) noexcept = default;

        ~BinaryData() override;

        BinaryData& operator=(const BinaryData&) = default;
        BinaryData& operator=(BinaryData&&) noexcept = default;

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext) override;
        std::vector<char> serialize() const override;

        BinaryData& operator=(const std::vector<char>& newValue);
        operator std::vector<char>() const;

        bool operator==(const std::vector<char>& rhsValue) const;
        bool operator!=(const std::vector<char>& rhsValue) const;

        void reset(std::size_t size = 0) override;

    private:
        UInt16 valueLength;
    };

    extern template class TypeBase<std::vector<char>>;

} // namespace iot::mqtt::types

#endif // IOT_MQTT_TYPES_BINARYDATA_H
