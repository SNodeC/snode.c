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

#include "iot/mqtt-fast/ControlPacket.h"

#include "iot/mqtt-fast/ControlPacketFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast {

    ControlPacket::ControlPacket(uint8_t type, uint8_t reserved)
        : type(type)
        , reserved(reserved) {
    }

    ControlPacket::ControlPacket(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : type(controlPacketFactory.getPacketType())
        , reserved(controlPacketFactory.getPacketFlags())
        , data(std::move(controlPacketFactory.getPacket())) {
    }

    uint8_t ControlPacket::getType() const {
        return type;
    }

    uint8_t ControlPacket::getReserved() const {
        return reserved;
    }

    uint64_t ControlPacket::getRemainingLength() const {
        return data.getLength();
    }

    std::vector<char> ControlPacket::getPacket() {
        std::vector<char> packet;

        packet.push_back(static_cast<char>((type << 0x04) | (reserved & 0x0F)));

        uint64_t remainingLength = data.getLength();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.getValue().begin(), data.getValue().end());

        return packet;
    }

    bool ControlPacket::isError() const {
        return error;
    }

    std::vector<char>& ControlPacket::getValue() {
        return data.getValue();
    }

    uint8_t ControlPacket::getInt8() {
        return data.getInt8();
    }

    uint16_t ControlPacket::getInt16() {
        return data.getInt16();
    }

    uint32_t ControlPacket::getInt32() {
        return data.getInt32();
    }

    uint64_t ControlPacket::getInt64() {
        return data.getInt64();
    }

    uint32_t ControlPacket::getIntV() {
        return data.getIntV();
    }

    std::string ControlPacket::getString() {
        return data.getString();
    }

    std::string ControlPacket::getStringRaw() {
        return data.getStringRaw();
    }

    std::list<uint8_t> ControlPacket::getUint8ListRaw() {
        return data.getUint8ListRaw();
    }

    void ControlPacket::putInt8(uint8_t value) {
        data.putInt8(value);
    }

    void ControlPacket::putInt16(uint16_t value) {
        data.putInt16(value);
    }

    void ControlPacket::putInt32(uint32_t value) {
        data.putInt32(value);
    }

    void ControlPacket::putInt64(uint64_t value) {
        data.putInt64(value);
    }

    void ControlPacket::putIntV(uint32_t value) {
        data.putIntV(value);
    }

    void ControlPacket::putString(const std::string& value) {
        data.putString(value);
    }

    void ControlPacket::putStringRaw(const std::string& value) {
        data.putStringRaw(value);
    }

    void ControlPacket::putUint8ListRaw(const std::list<uint8_t>& value) {
        data.putUint8ListRaw(value);
    }

    bool ControlPacket::isError() {
        return data.isError();
    }

} // namespace iot::mqtt_fast
