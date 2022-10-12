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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : iot::mqtt::SocketContext(socketConnection) {
    }

    iot::mqtt::ControlPacketDeserializer* SocketContext::onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) {
        iot::mqtt::ControlPacketDeserializer* currentPacket = nullptr;

        switch (staticHeader.getPacketType()) {
            case MQTT_CONNACK:
                currentPacket = new iot::mqtt::packets::deserializer::Connack(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_SUBACK:
                currentPacket = new iot::mqtt::packets::deserializer::Suback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_UNSUBACK:
                currentPacket = new iot::mqtt::packets::deserializer::Unsuback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PINGRESP:
                currentPacket = new iot::mqtt::packets::deserializer::Pingresp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PUBLISH:
                currentPacket = new iot::mqtt::packets::deserializer::Publish(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PUBACK:
                currentPacket = new iot::mqtt::packets::deserializer::Puback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PUBREC:
                currentPacket = new iot::mqtt::packets::deserializer::Pubrec(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PUBREL:
                currentPacket = new iot::mqtt::packets::deserializer::Pubrel(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            case MQTT_PUBCOMP:
                currentPacket = new iot::mqtt::packets::deserializer::Pubcomp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                break;
            default:
                currentPacket = nullptr;
                break;
        }

        return currentPacket;
    }

    void SocketContext::_onConnack(iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            shutdown(true);
        } else {
            onConnack(connack);
        }
    }

    void SocketContext::__onPublish(mqtt::packets::Publish& publish) {
        onPublish(publish);
    }

    void SocketContext::_onPublish(packets::Publish& publish) {
        if (publish.getQoSLevel() > 2) {
            shutdown(true);
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoSLevel() > 0) {
            shutdown(true);
        } else {
            __onPublish(publish);

            switch (publish.getQoSLevel()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());
                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());
                    break;
            }
        }
    }

    void SocketContext::_onPuback(packets::Puback& puback) {
        if (puback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPuback(puback);
        }
    }

    void SocketContext::_onPubrec(packets::Pubrec& pubrec) {
        if (pubrec.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            sendPubrel(pubrec.getPacketIdentifier());

            onPubrec(pubrec);
        }
    }

    void SocketContext::_onPubrel(packets::Pubrel& pubrel) {
        if (pubrel.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubrel(pubrel);

            sendPubcomp(pubrel.getPacketIdentifier());
        }
    }

    void SocketContext::_onPubcomp(packets::Pubcomp& pubcomp) {
        if (pubcomp.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubcomp(pubcomp);
        }
    }

    void SocketContext::_onSuback(iot::mqtt::packets::Suback& suback) {
        if (suback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSuback(suback);
        }
    }

    void SocketContext::_onUnsuback(iot::mqtt::packets::Unsuback& unsuback) {
        if (unsuback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onUnsuback(unsuback);
        }
    }

    void SocketContext::_onPingresp(iot::mqtt::packets::Pingresp& pingresp) {
        onPingresp(pingresp);
    }

    void SocketContext::sendConnect(const std::string& clientId) { // Client
        LOG(DEBUG) << "Send CONNECT";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Connect(0x00 /* connect flags */,
                                         60 /* keep alive */,
                                         clientId,
                                         "" /* WillTopic */,
                                         "" /* WillMessage */,
                                         "" /* Username */,
                                         "" /* Password */)); // Flags, Username, Will, ...
    }

    void SocketContext::sendSubscribe(std::list<iot::mqtt::Topic>& topics) { // Client
        LOG(DEBUG) << "Send SUBSCRIBE";
        LOG(DEBUG) << "==============";

        send(iot::mqtt::packets::Subscribe(0, topics));
    }

    void SocketContext::sendUnsubscribe(std::list<std::string>& topics) { // Client
        LOG(DEBUG) << "Send UNSUBSCRIBE";
        LOG(DEBUG) << "================";

        send(iot::mqtt::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendPingreq() { // Client
        LOG(DEBUG) << "Send Pingreq";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Pingreq());
    }

    void SocketContext::sendDisconnect() {
        LOG(DEBUG) << "Send Disconnect";
        LOG(DEBUG) << "===============";

        send(iot::mqtt::packets::Disconnect());
    }

} // namespace iot::mqtt::client
