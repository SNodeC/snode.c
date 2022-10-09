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

#include "iot/mqtt/StaticHeader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <iomanip>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : iot::mqtt::SocketContext(socketConnection) {
    }

    iot::mqtt::ControlPacketReceiver* SocketContext::onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) {
        LOG(TRACE) << "======================================================";
        LOG(TRACE) << "PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(staticHeader.getPacketType());
        LOG(TRACE) << "PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(staticHeader.getFlags());
        LOG(TRACE) << "RemainingLength: " << staticHeader.getRemainingLength();

        iot::mqtt::ControlPacketReceiver* currentPacket = nullptr;

        switch (staticHeader.getPacketType()) {
            case MQTT_CONNECT: // Server
                currentPacket = new iot::mqtt::server::packets::Connect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_SUBSCRIBE: // Server
                currentPacket = new iot::mqtt::server::packets::Subscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_UNSUBSCRIBE: // Server
                currentPacket = new iot::mqtt::server::packets::Unsubscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            /*
            case MQTT_PINGREQ: //
                currentPacket = new iot::mqtt::server::packets::Pingreq(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_DISCONNECT: // Server
                currentPacket = new iot::mqtt::server::packets::Disconnect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;

            */
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
            onSubscribe(subscribe); // Shall only subscribe but not send retained messages

            std::list<uint8_t> returnCodes;

            // Check QoS-Levels of subscribtions (topic.getRequestedQoS())
            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                returnCodes.push_back(topic.getAcceptedQoS()); // QoS + Success
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);

            // Here we shall trigger sending of retained messages
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

    /*
        void SocketContext::_onPingreq(packets::Pingreq& pingreq) {
            sendPingresp();

            onPingreq(pingreq);
        }

        void SocketContext::_onDisconnect(packets::Disconnect& disconnect) {
            onDisconnect(disconnect);

            shutdown();
        }
    */

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) { // Server
        LOG(TRACE) << "Send CONNACK";
        LOG(TRACE) << "============";

        send(iot::mqtt::server::packets::Connack(returnCode, flags));

        if (returnCode != MQTT_CONNACK_ACCEPT) {
            shutdown();
        }
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) { // Server
        LOG(TRACE) << "Send SUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt::server::packets::Suback(packetIdentifier, returnCodes));
    }

    /*

        void SocketContext::sendUnsuback(uint16_t packetIdentifier) { // Server
            LOG(TRACE) << "Send UNSUBACK";
            LOG(TRACE) << "=============";

            send(iot::mqtt::packets::Unsuback(packetIdentifier));
        }

        void SocketContext::sendPingresp() { // Server
            LOG(TRACE) << "Send Pingresp";
            LOG(TRACE) << "=============";

            send(iot::mqtt::packets::Pingresp());
        }
    */

} // namespace iot::mqtt::server
