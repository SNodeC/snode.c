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
