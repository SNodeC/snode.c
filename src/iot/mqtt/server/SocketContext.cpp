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

#include "iot/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : iot::mqtt::SocketContext(socketConnection) {
    }

    iot::mqtt::ControlPacketReceiver* SocketContext::deserialize(iot::mqtt::StaticHeader& staticHeader) {
        iot::mqtt::ControlPacketReceiver* currentPacket = nullptr;

        switch (staticHeader.getPacketType()) {
            case MQTT_CONNECT:
                currentPacket = new iot::mqtt::server::packets::Connect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_SUBSCRIBE:
                currentPacket = new iot::mqtt::server::packets::Subscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_UNSUBSCRIBE:
                currentPacket = new iot::mqtt::server::packets::Unsubscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PINGREQ:
                currentPacket = new iot::mqtt::server::packets::Pingreq(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_DISCONNECT:
                currentPacket = new iot::mqtt::server::packets::Disconnect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            default:
                currentPacket = nullptr;
                break;
        }

        return currentPacket;
    }

    void SocketContext::_onConnect(packets::Connect& connect) {
        if (connect.getProtocol() != "MQTT") {
            shutdown(true);
        } else if (connect.getLevel() != MQTT_VERSION_3_1_1) {
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);
            shutdown(true);

        } else if (connect.getClientId() == "" && !connect.getCleanSession()) {
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);
            shutdown(true);

        } else {
            if (connect.getClientId().empty()) {
                connect.setClientId(getRandomClientId());
            }

            onConnect(connect);
        }
    }

    void SocketContext::_onSubscribe(packets::Subscribe& subscribe) {
        if (subscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSubscribe(subscribe);

            std::list<uint8_t> returnCodes;

            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                returnCodes.push_back(topic.getAcceptedQoS());
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);
        }
    }

    void SocketContext::_onUnsubscribe(packets::Unsubscribe& unsubscribe) {
        if (unsubscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            sendUnsuback(unsubscribe.getPacketIdentifier());

            onUnsubscribe(unsubscribe);
        }
    }

    void SocketContext::_onPingreq(packets::Pingreq& pingreq) {
        sendPingresp();

        onPingreq(pingreq);
    }

    void SocketContext::_onDisconnect(packets::Disconnect& disconnect) {
        onDisconnect(disconnect);

        shutdown();
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) {
        LOG(TRACE) << "Send CONNACK";
        LOG(TRACE) << "============";

        send(iot::mqtt::server::packets::Connack(returnCode, flags));

        if (returnCode != MQTT_CONNACK_ACCEPT) {
            shutdown();
        }
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        LOG(TRACE) << "Send SUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt::server::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send UNSUBACK";
        LOG(TRACE) << "=============";

        send(iot::mqtt::server::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingresp() { // Server
        LOG(TRACE) << "Send Pingresp";
        LOG(TRACE) << "=============";

        send(iot::mqtt::server::packets::Pingresp());
    }

} // namespace iot::mqtt::server
