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

#ifndef IOT_MQTTFAST_CONTROLPACKET_H
#define IOT_MQTTFAST_CONTROLPACKET_H

#include "iot/mqtt-fast/types/Binary.h"

namespace iot::mqtt_fast {
    class ControlPacketFactory;
} // namespace iot::mqtt_fast

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast {

    class ControlPacket {
    public:
        explicit ControlPacket(uint8_t type, uint8_t reserved = 0);
        explicit ControlPacket(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory);

        ControlPacket(const ControlPacket&) = delete;
        ControlPacket(ControlPacket&&) = delete;

        ControlPacket& operator=(const ControlPacket&) = delete;
        ControlPacket& operator=(ControlPacket&&) = delete;

        uint8_t getType() const;
        uint8_t getReserved() const;
        uint64_t getRemainingLength() const;
        std::vector<char> getPacket();

        bool isError() const;

    protected:
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
        void putString(const std::string& value);
        void putStringRaw(const std::string& value);
        void putUint8ListRaw(const std::list<uint8_t>& value);

        bool isError();

    private:
        uint8_t type;
        uint8_t reserved;

        iot::mqtt_fast::types::Binary data;

    protected:
        bool error = false;
    };

} // namespace iot::mqtt_fast

#endif // IOT_MQTTFAST_CONTROLPACKET_H
