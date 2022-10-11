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

#include "iot/mqtt/packets/deserializer/Connect.h"

#include "iot/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets::deserializer {

    Connect::Connect(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNECT, flags)
        , iot::mqtt::ControlPacketReceiver(remainingLength, MQTT_CONNECT_FLAGS) {
    }

    std::size_t Connect::deserializeVP(iot::mqtt::SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += protocol.deserialize(socketContext);
                if (!protocol.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += level.deserialize(socketContext);
                if (!level.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 2:
                consumed += connectFlags.deserialize(socketContext);
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
                consumed += keepAlive.deserialize(socketContext);
                if (!keepAlive.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 4: // Payload
                consumed += clientId.deserialize(socketContext);
                if (!clientId.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 5:
                if (willFlag) {
                    consumed += willTopic.deserialize(socketContext);
                    if (!willTopic.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 6:
                if (willFlag) {
                    consumed += willMessage.deserialize(socketContext);
                    if (!willMessage.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 7:
                if (usernameFlag) {
                    consumed += username.deserialize(socketContext);
                    if (!username.isComplete()) {
                        break;
                    }
                }

                state++;
                [[fallthrough]];
            case 8:
                if (passwordFlag) {
                    consumed += password.deserialize(socketContext);
                    if (!username.isComplete()) {
                        break;
                    }
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Connect::propagateEvent(iot::mqtt::SocketContext* socketContext) {
        dynamic_cast<iot::mqtt::server::SocketContext*>(socketContext)->_onConnect(*this);
    }

} // namespace iot::mqtt::packets::deserializer
