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

#include "iot/mqtt1/packets/Connect.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::packets {

    Connect::Connect(const std::string& clientId)
        : iot::mqtt1::ControlPacket(MQTT_CONNECT, 0, 0) {
        _clientId.setValue(clientId);
    }

    Connect::Connect(uint32_t remainingLength, uint8_t reserved)
        : iot::mqtt1::ControlPacket(MQTT_CONNECT, reserved, remainingLength) {
    }

    std::string Connect::getProtocol() const {
        return _protocol.getValue();
    }

    uint8_t Connect::getLevel() const {
        return _level.getValue();
    }

    uint8_t Connect::getFlags() const {
        return flags;
    }

    uint16_t Connect::getKeepAlive() const {
        return _keepAlive.getValue();
    }

    std::string Connect::getClientId() const {
        return _clientId.getValue();
    }

    bool Connect::getUsernameFlag() const {
        return usernameFlag;
    }

    bool Connect::getPasswordFlag() const {
        return passwordFlag;
    }

    bool Connect::getWillRetain() const {
        return willRetain;
    }

    uint8_t Connect::getWillQoS() const {
        return willQoS;
    }

    bool Connect::getWillFlag() const {
        return willFlag;
    }

    bool Connect::getCleanSession() const {
        return cleanSession;
    }

    std::string Connect::getWillTopic() const {
        return _willTopic.getValue();
    }

    std::string Connect::getWillMessage() const {
        return _willMessage.getValue();
    }

    std::string Connect::getUsername() const {
        return _username.getValue();
    }

    std::string Connect::getPassword() const {
        return _password.getValue();
    }

    std::vector<char> Connect::getPacket() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = _protocol.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = _level.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = _flags.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = _keepAlive.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = _clientId.getValueAsVector();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        if (willFlag) {
            tmpVector = _willTopic.getValueAsVector();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

            tmpVector = _willMessage.getValueAsVector();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }
        if (usernameFlag) {
            tmpVector = _username.getValueAsVector();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }
        if (passwordFlag) {
            tmpVector = _password.getValueAsVector();
            packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());
        }

        return packet;
    }

    std::size_t Connect::construct(SocketContext* socketContext) {
        std::size_t consumedTotal = 0;
        std::size_t consumed = 0;

        switch (state) {
            // V-Header
            case 0:
                consumed = _protocol.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _protocol.isError()) || !_protocol.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 1:
                consumed = _level.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _level.isError()) || !_level.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 2:
                consumed = _flags.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _flags.isError()) || !_flags.isComplete()) {
                    break;
                }

                flags = _flags.getValue();

                reserved = (flags & 0x01) != 0;
                cleanSession = (flags & 0x02) != 0;
                willFlag = (flags & 0x04) != 0;
                willQoS = (flags & 0x18) >> 3;
                willRetain = (flags & 0x20) != 0;
                passwordFlag = (flags & 0x40) != 0;
                usernameFlag = (flags & 0x80) != 0;
                state++;
                [[fallthrough]];
            case 3:
                consumed = _keepAlive.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _keepAlive.isError()) || !_keepAlive.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            // Payload
            case 4:
                consumed = _clientId.construct(socketContext);
                consumedTotal += consumed;

                if (consumed == 0 || (error = _clientId.isError()) || !_clientId.isComplete()) {
                    break;
                }
                state++;
                [[fallthrough]];
            case 5:
                if (willFlag) {
                    consumed = _willTopic.construct(socketContext);
                    consumedTotal += consumed;

                    if (consumed == 0 || (error = _willTopic.isError()) || !_willTopic.isComplete()) {
                        break;
                    }
                }
                state++;
                [[fallthrough]];
            case 6:
                if (willFlag) {
                    consumed = _willMessage.construct(socketContext);
                    consumedTotal += consumed;

                    if (consumed == 0 || (error = _willMessage.isError()) || !_willMessage.isComplete()) {
                        break;
                    }
                }
                state++;
                [[fallthrough]];
            case 7:
                if (usernameFlag) {
                    consumed = _username.construct(socketContext);
                    consumedTotal += consumed;

                    if (consumed == 0 || (error = _username.isError()) || !_username.isComplete()) {
                        break;
                    }
                }
                state++;
                [[fallthrough]];
            case 8:
                if (passwordFlag) {
                    consumed = _password.construct(socketContext);
                    consumedTotal += consumed;

                    if (consumed == 0 || (error = _password.isError()) || !_password.isComplete()) {
                        break;
                    }
                }
                [[fallthrough]];
            default:
                complete = true;
                break;
        }

        return consumedTotal;
    }

    void Connect::propagateEvent(SocketContext* socketContext) const {
        socketContext->_onConnect(*this);
    }

} // namespace iot::mqtt1::packets
