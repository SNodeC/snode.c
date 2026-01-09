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

#include "iot/mqtt/FixedHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    FixedHeader::FixedHeader() {
    }

    FixedHeader::FixedHeader(uint8_t type, uint8_t flags, uint32_t remainingLength) {
        this->typeFlags = static_cast<uint8_t>((type << 4) | (flags & 0x0F));
        this->remainingLength = remainingLength;
    }

    FixedHeader::~FixedHeader() {
    }

    std::size_t FixedHeader::deserialize(MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed += typeFlags.deserialize(mqttContext);

                if (!typeFlags.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 1:
                consumed += remainingLength.deserialize(mqttContext);

                complete = remainingLength.isComplete();
                error = remainingLength.isError();

                break;
        }

        return consumed;
    }

    uint8_t FixedHeader::getType() const {
        return static_cast<uint8_t>(typeFlags >> 0x04);
    }

    uint8_t FixedHeader::getFlags() const {
        return static_cast<uint8_t>(typeFlags & 0x0F);
    }

    void FixedHeader::setRemainingLength(uint32_t remainingLength) {
        this->remainingLength = remainingLength;
    }

    uint32_t FixedHeader::getRemainingLength() const {
        return remainingLength;
    }

    bool FixedHeader::isComplete() const {
        return complete;
    }

    bool FixedHeader::isError() const {
        return error;
    }

    std::vector<char> FixedHeader::serialize() const {
        std::vector<char> packet = typeFlags.serialize();

        std::vector<char> tmpVector = remainingLength.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    void FixedHeader::reset() {
        typeFlags.reset();
        remainingLength.reset();

        complete = false;
        error = false;

        state = 0;
    }

} // namespace iot::mqtt
