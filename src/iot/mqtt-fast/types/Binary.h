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
