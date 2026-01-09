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

#include "iot/mqtt-fast/packets/Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::packets {

    Unsubscribe::Unsubscribe(uint16_t packetIdentifier, const std::list<std::string>& topics)
        : iot::mqtt_fast::ControlPacket(MQTT_UNSUBSCRIBE, 0x02)
        , packetIdentifier(packetIdentifier)
        , topics(topics) {
        // V-Header
        putInt16(this->packetIdentifier);

        // Payload
        for (const std::string& topic : this->topics) {
            putString(topic);
        }
    }

    Unsubscribe::Unsubscribe(iot::mqtt_fast::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt_fast::ControlPacket(controlPacketFactory) {
        // V-Header
        packetIdentifier = getInt16();

        // Payload
        for (std::string name = getString(); !name.empty(); name = getString()) {
            topics.push_back(name);
        }

        if (!isError()) {
            error = topics.empty();
        }
    }

    uint16_t Unsubscribe::getPacketIdentifier() const {
        return packetIdentifier;
    }

    const std::list<std::string>& Unsubscribe::getTopics() const {
        return topics;
    }

} // namespace iot::mqtt_fast::packets
