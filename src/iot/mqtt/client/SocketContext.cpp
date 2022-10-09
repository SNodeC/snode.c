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

#include "iot/mqtt/client/SocketContext.h"

#include "iot/mqtt/StaticHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : iot::mqtt::SocketContext(socketConnection) {
    }

    iot::mqtt::ControlPacketReceiver* SocketContext::onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) {
        iot::mqtt::ControlPacketReceiver* currentPacket = nullptr;

        switch (staticHeader.getPacketType()) {
            case MQTT_CONNACK: // Client
                currentPacket = new iot::mqtt::client::packets::Connack(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_SUBACK: // Client
                currentPacket = new iot::mqtt::client::packets::Suback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_UNSUBACK: // Client
                currentPacket = new iot::mqtt::client::packets::Unsuback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PINGRESP: // Client
                currentPacket = new iot::mqtt::client::packets::Pingresp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            default:
                currentPacket = nullptr;
                break;
        }

        return currentPacket;
    }

    void SocketContext::_onConnack(packets::Connack& connack) {
        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            shutdown(true);
        } else {
            onConnack(connack);
        }
    }

    void SocketContext::_onSuback(packets::Suback& suback) {
        if (suback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSuback(suback);
        }
    }

    void SocketContext::_onUnsuback(packets::Unsuback& unsuback) {
        if (unsuback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onUnsuback(unsuback);
        }
    }

    void SocketContext::_onPingresp(packets::Pingresp& pingresp) {
        onPingresp(pingresp);
    }

    void SocketContext::sendConnect(const std::string& clientId) { // Client
        LOG(TRACE) << "Send CONNECT";
        LOG(TRACE) << "============";

        send(iot::mqtt::client::packets::Connect(clientId)); // Flags, Username, Will, ...
    }

    void SocketContext::sendSubscribe(std::list<iot::mqtt::Topic>& topics) { // Client
        LOG(TRACE) << "Send SUBSCRIBE";
        LOG(TRACE) << "==============";

        send(iot::mqtt::client::packets::Subscribe(0, topics));
    }

    void SocketContext::sendUnsubscribe(std::list<std::string>& topics) { // Client
        LOG(TRACE) << "Send UNSUBSCRIBE";
        LOG(TRACE) << "================";

        send(iot::mqtt::client::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendPingreq() { // Client
        LOG(TRACE) << "Send Pingreq";
        LOG(TRACE) << "============";

        send(iot::mqtt::client::packets::Pingreq());
    }

    void SocketContext::sendDisconnect() {
        LOG(TRACE) << "Send Disconnect";
        LOG(TRACE) << "===============";

        send(iot::mqtt::client::packets::Disconnect());
    }

} // namespace iot::mqtt::client
