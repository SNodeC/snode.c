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

#include "iot/mqtt/server/packets/Publish.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::packets {

    Publish::Publish(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::server::ControlPacketDeserializer(remainingLength, flags, flags) {
        this->flags = flags;
        this->qoS = flags >> 1 & 0x03;
        this->dup = (flags & 0x08) != 0;
        this->retain = (flags & 0x01) != 0;

        error = this->qoS > 2;
    }

    std::size_t Publish::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += topic.deserialize(mqttContext);
                if (!topic.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                if (qoS > 0) {
                    consumed += packetIdentifier.deserialize(mqttContext);
                    if (!packetIdentifier.isComplete()) {
                        break;
                    }
                }

                message.setSize(static_cast<uint16_t>(getRemainingLength() - getConsumed() - consumed));

                state++;
                [[fallthrough]];
            case 2: // Payload
                consumed += message.deserialize(mqttContext);
                if (!message.isComplete()) {
                    break;
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Publish::deliverPacket(iot::mqtt::server::Mqtt* mqtt) {
        mqtt->printVP(*this);
        mqtt->_onPublish(*this);
    }

} // namespace iot::mqtt::server::packets
