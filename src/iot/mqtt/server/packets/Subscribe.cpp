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

#include "iot/mqtt/server/packets/Subscribe.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::packets {

    Subscribe::Subscribe(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::server::ControlPacketDeserializer(remainingLength, flags, MQTT_SUBSCRIBE_FLAGS) {
        this->flags = flags;
    }

    std::size_t Subscribe::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += packetIdentifier.deserialize(mqttContext);

                if (!packetIdentifier.isComplete()) {
                    break;
                }

                if (mqttContext->getProtocolLevel() == MQTT_VERSION_5_0) {
                    state++;
                } else {
                    state = 2;
                }
                [[fallthrough]];
            case 1:
                if (mqttContext->getProtocolLevel() == MQTT_VERSION_5_0) {
                    consumed += propertiesLength.deserialize(mqttContext);
                    if (!propertiesLength.isComplete()) {
                        break;
                    }

                    propertiesRemaining = propertiesLength;

                    while (propertiesRemaining > 0) {
                        char buffer[128];
                        const std::size_t chunk =
                            propertiesRemaining < sizeof(buffer) ? propertiesRemaining : sizeof(buffer);
                        const std::size_t read = mqttContext->recv(buffer, chunk);
                        consumed += read;

                        if (read == 0) {
                            break;
                        }

                        propertiesRemaining -= read;
                    }

                    if (propertiesRemaining > 0) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 2: // Payload
                consumed += topic.deserialize(mqttContext);

                if (!topic.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 3:
                consumed += qoS.deserialize(mqttContext);

                if (!qoS.isComplete()) {
                    break;
                }
                topics.emplace_back(topic, qoS);
                topic.reset();
                qoS.reset();

                if (getConsumed() + consumed < this->getRemainingLength()) {
                    state = 2;
                }

                complete = getConsumed() + consumed >= this->getRemainingLength();

                break;
        }

        return consumed;
    }

    void Subscribe::deliverPacket(iot::mqtt::server::Mqtt* mqtt) {
        mqtt->printVP(*this);
        mqtt->_onSubscribe(*this);
    }

} // namespace iot::mqtt::server::packets
