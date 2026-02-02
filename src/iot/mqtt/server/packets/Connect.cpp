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

#include "iot/mqtt/server/packets/Connect.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Uuid.h"

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::packets {

    Connect::Connect(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::server::ControlPacketDeserializer(remainingLength, flags, MQTT_CONNECT_FLAGS) {
        this->flags = flags;
    }

    std::size_t Connect::deserializeVP(iot::mqtt::MqttContext* mqttContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += protocol.deserialize(mqttContext);
                if (!protocol.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += level.deserialize(mqttContext);
                if (!level.isComplete()) {
                    break;
                }
                reflect = (level & 0x80) == 0; // msb in level == 1 -> do not reflect message to origin (try_private in mosquitto)
                level = level & ~0x80;

                state++;
                [[fallthrough]];
            case 2:
                consumed += connectFlags.deserialize(mqttContext);
                if (!connectFlags.isComplete()) {
                    break;
                }

                reserved = (connectFlags & 0x01) != 0;
                cleanSession = (connectFlags & 0x02) != 0;
                willFlag = (connectFlags & 0x04) != 0;
                willQoS = (connectFlags & 0x18) >> 3;
                willRetain = (connectFlags & 0x20) != 0;
                passwordFlag = (connectFlags & 0x40) != 0;
                usernameFlag = (connectFlags & 0x80) != 0;

                state++;
                [[fallthrough]];
            case 3:
                consumed += keepAlive.deserialize(mqttContext);
                if (!keepAlive.isComplete()) {
                    break;
                }

                if (level == MQTT_VERSION_5_0) {
                    state++;
                } else {
                    state = 6;
                }
                [[fallthrough]];
            case 4: // Properties length (MQTT 5.0)
                if (level != MQTT_VERSION_5_0) {
                    state = 6;
                    [[fallthrough]];
                } else {
                    consumed += propertiesLength.deserialize(mqttContext);
                    if (!propertiesLength.isComplete()) {
                        break;
                    }

                    propertiesRemaining = propertiesLength;
                    hasPropertiesFlag = propertiesRemaining > 0;

                    state++;
                    [[fallthrough]];
                }
            case 5: // Properties (MQTT 5.0)
                if (level == MQTT_VERSION_5_0 && propertiesRemaining > 0) {
                    char buffer[128];

                    while (propertiesRemaining > 0) {
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
            case 6: // Payload
                consumed += clientId.deserialize(mqttContext);
                if (!clientId.isComplete()) {
                    break;
                }

                if (clientId.size() == 0) {
                    clientId = utils::Uuid::getUuid();
                    fakedClientId = true;
                }

                state++;
                [[fallthrough]];
            case 7:
                if (willFlag && level == MQTT_VERSION_5_0) {
                    consumed += willPropertiesLength.deserialize(mqttContext);
                    if (!willPropertiesLength.isComplete()) {
                        break;
                    }

                    willPropertiesRemaining = willPropertiesLength;
                    hasWillPropertiesFlag = willPropertiesRemaining > 0;
                }

                state++;
                [[fallthrough]];
            case 8:
                if (willFlag && level == MQTT_VERSION_5_0 && willPropertiesRemaining > 0) {
                    char buffer[128];

                    while (willPropertiesRemaining > 0) {
                        const std::size_t chunk =
                            willPropertiesRemaining < sizeof(buffer) ? willPropertiesRemaining : sizeof(buffer);
                        const std::size_t read = mqttContext->recv(buffer, chunk);
                        consumed += read;

                        if (read == 0) {
                            break;
                        }

                        willPropertiesRemaining -= read;
                    }

                    if (willPropertiesRemaining > 0) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 9:
                if (willFlag) {
                    consumed += willTopic.deserialize(mqttContext);
                    if (!willTopic.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 10:
                if (willFlag) {
                    consumed += willMessage.deserialize(mqttContext);
                    if (!willMessage.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 11:
                if (usernameFlag) {
                    consumed += username.deserialize(mqttContext);
                    if (!username.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 12:
                if (passwordFlag) {
                    consumed += password.deserialize(mqttContext);
                    if (!password.isComplete()) {
                        break;
                    }
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Connect::deliverPacket(iot::mqtt::server::Mqtt* mqtt) {
        mqtt->printVP(*this);
        mqtt->_onConnect(*this);
    }

    bool Connect::isFakedClientId() const {
        return fakedClientId;
    }

    bool Connect::hasProperties() const {
        return hasPropertiesFlag;
    }

    bool Connect::hasWillProperties() const {
        return hasWillPropertiesFlag;
    }

} // namespace iot::mqtt::server::packets
