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

#include "iot/mqtt/types/Binary.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    Binary::Binary(iot::mqtt::SocketContext* socketContext)
        : iot::mqtt::types::TypeBase(socketContext) {
    }

    Binary::~Binary() {
    }

    void Binary::setLength(std::vector<char>::size_type length) {
        this->length = this->needed = static_cast<std::vector<char>::size_type>(length);

        data.resize(static_cast<std::vector<char>::size_type>(length), '\0');
    }

    std::vector<char>::size_type Binary::getLength() const {
        return length;
    }

    std::size_t Binary::construct() {
        std::size_t consumed = 0;

        consumed = read(data.data() + length - needed, static_cast<std::size_t>(needed));
        needed -= consumed;
        completed = needed == 0;

        return consumed;
    }

    std::vector<char>& Binary::getValue() {
        return data;
    }

    uint8_t Binary::getInt8() {
        uint8_t ret = 0;

        if (!error && pointer + sizeof(uint8_t) < length + 1) {
            ret = *reinterpret_cast<uint8_t*>(data.data() + pointer);
            pointer += sizeof(uint8_t);
        } else {
            error = true;
        }

        return ret;
    }

    uint16_t Binary::getInt16() {
        uint16_t ret = 0;

        if (!error && pointer + sizeof(uint16_t) < length + 1) {
            ret = be16toh(*reinterpret_cast<uint16_t*>(data.data() + pointer));
            pointer += sizeof(uint16_t);
        } else {
            error = true;
        }

        return ret;
    }

    uint32_t Binary::getInt32() {
        uint32_t ret = 0;

        if (!error && pointer + sizeof(uint32_t) < length + 1) {
            ret = be32toh(*reinterpret_cast<uint32_t*>(data.data() + pointer));
            pointer += sizeof(uint32_t);
        } else {
            error = true;
        }

        return ret;
    }

    uint64_t Binary::getInt64() {
        uint64_t ret = 0;

        if (!error && pointer + sizeof(uint64_t) < length + 1) {
            ret = be64toh(*reinterpret_cast<uint64_t*>(data.data() + pointer));
            pointer += sizeof(uint64_t);
        } else {
            error = true;
        }

        return ret;
    }

    uint32_t Binary::getIntV() {
        uint32_t value = 0;
        uint32_t multiplier = 1;

        do {
            if (!error && pointer + sizeof(uint8_t) < length + 1) {
                uint8_t byte = *reinterpret_cast<uint8_t*>(data.data() + pointer);
                pointer += sizeof(uint8_t);

                value += (byte & 0x7F) * multiplier;
                if (multiplier > 0x80 * 0x80 * 0x80) {
                    error = true;
                } else {
                    multiplier *= 0x80;
                    completed = (byte & 0x80) == 0;
                }
            } else {
                error = true;
            }
        } while (!completed && !error);

        return value;
    }

    std::string Binary::getString() {
        std::string string = "";

        if (!error && pointer + sizeof(uint16_t) < length + 1) {
            uint16_t stringLen = getInt16();

            if (pointer + stringLen < length + 1) {
                string = std::string(data.data() + pointer, stringLen);
                pointer += stringLen;
            } else {
                error = true;
            }
        } else {
            error = true;
        }

        return string;
    }

    std::string Binary::getStringRaw() {
        std::string string = "";

        if (!error && length - pointer > 0) {
            string = std::string(data.data() + pointer, length - pointer);
            pointer += length - pointer;
        } else {
            error = true;
        }

        return string;
    }

    std::list<uint8_t> Binary::getUint8ListRaw() {
        std::list<uint8_t> uint8List;

        if (!error && length - pointer > 0) {
            for (; pointer < length; ++pointer) {
                uint8List.push_back(*reinterpret_cast<uint8_t*>((data.data() + pointer)));
            }
        } else {
            error = true;
        }

        return uint8List;
    }

    void Binary::putInt8(uint8_t value) {
        data.push_back(static_cast<char>(value));
        length += sizeof(uint8_t);
    }

    void Binary::putInt16(uint16_t value) {
        data.push_back(static_cast<char>(value >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(value & 0xFF));
        length += sizeof(uint16_t);
    }

    void Binary::putInt32(uint32_t value) {
        data.push_back(static_cast<char>(value >> 0x18 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x10 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(value & 0xFF));
        length += sizeof(uint32_t);
    }

    void Binary::putInt64(uint64_t value) {
        data.push_back(static_cast<char>(value >> 0x38 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x30 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x28 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x20 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x18 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x10 & 0xFF));
        data.push_back(static_cast<char>(value >> 0x08 & 0xFF));
        data.push_back(static_cast<char>(value & 0xFF));
        length += sizeof(uint64_t);
    }

    void Binary::putIntV(uint32_t value) {
        do {
            uint8_t encodedByte = static_cast<uint8_t>(value % 0x80);
            value /= 0x80;
            if (value > 0) {
                encodedByte |= 0x80;
            }
            putInt8(encodedByte);
        } while (value > 0);
    }

    void Binary::putString(const std::string& string) {
        uint16_t stringLen = static_cast<uint16_t>(string.length());
        putInt16(stringLen);

        putStringRaw(string);
    }

    void Binary::putStringRaw(const std::string& string) {
        data.insert(data.end(), string.begin(), string.end());
        length += string.length();
    }

    void Binary::reset() {
        pointer = 0;
        length = 0;
        needed = 0;
        data.clear();

        iot::mqtt::types::TypeBase::reset();
    }

} // namespace iot::mqtt::types
