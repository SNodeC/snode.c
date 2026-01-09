/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#ifndef IOT_MQTTFAST_TYPES_BINARY_H
#define IOT_MQTTFAST_TYPES_BINARY_H

#include "iot/mqtt-fast/types/TypeBase.h"

namespace core::socket {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::types {

    class Binary : public iot::mqtt_fast::types::TypeBase {
    public:
        explicit Binary(core::socket::SocketContext* socketContext = nullptr);

        void setLength(std::vector<char>::size_type length);
        std::vector<char>::size_type getLength() const;

        std::size_t construct() override;

        std::vector<char>& getValue();

        uint8_t getInt8();
        uint16_t getInt16();
        uint32_t getInt32();
        uint64_t getInt64();
        uint32_t getIntV();
        std::string getString();
        std::string getStringRaw();
        std::list<uint8_t> getUint8ListRaw();

        void putInt8(uint8_t value);
        void putInt16(uint16_t value);
        void putInt32(uint32_t value);
        void putInt64(uint64_t value);
        void putIntV(uint32_t value);
        void putString(const std::string& string);
        void putStringRaw(const std::string& string);
        void putUint8ListRaw(const std::list<uint8_t>& values);

        void reset() override;

    private:
        std::vector<char>::size_type pointer = 0;
        std::vector<char>::size_type length = 0;
        std::vector<char>::size_type needed = 0;

        std::vector<char> binary;
    };

} // namespace iot::mqtt_fast::types

#endif // IOT_MQTTFAST_TYPES_BINARY_H
