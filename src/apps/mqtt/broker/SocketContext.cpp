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

#include "apps/mqtt/broker/SocketContext.h"

#include "apps/mqtt/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<Broker> broker)
        : iot::mqtt::SocketContext(socketConnection)
        , broker(broker) {
    }

    SocketContext::~SocketContext() {
        if (subscribtionCount > 0) {
            broker->unsubscribe(this);
        }
    }

    void SocketContext::onConnect(const iot::mqtt::packets::Connect& connect) {
        VLOG(0) << "CONNECT";
        VLOG(0) << "=======";
        VLOG(0) << "Error: " << connect.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(connect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connect.getReserved());
        VLOG(0) << "RemainingLength: " << connect.getRemainingLength();
        VLOG(0) << "Protocol: " << connect.getProtocol();
        VLOG(0) << "Version: " << static_cast<uint16_t>(connect.getVersion());
        VLOG(0) << "ConnectFlags: " << static_cast<uint16_t>(connect.getFlags());
        VLOG(0) << "KeepAlive: " << connect.getKeepAlive();
        VLOG(0) << "ClientID: " << connect.getClientId();

        sendConnack(connect.getVersion() <= 0x04 ? MQTT_CONNACK_ACCEPT : MQTT_CONNACK_UNACEPTABLEVERSION); // Version | 0x04 = 3.1.1
    }

    void SocketContext::onConnack(const iot::mqtt::packets::Connack& connack) {
        VLOG(0) << "CONNACK";
        VLOG(0) << "=======";
        VLOG(0) << "Error: " << connack.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(connack.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connack.getReserved());
        VLOG(0) << "RemainingLength: " << connack.getRemainingLength();
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connack.getFlags());
        VLOG(0) << "Reason: " << connack.getReason();
    }

    void SocketContext::onPublish(const iot::mqtt::packets::Publish& publish) {
        VLOG(0) << "PUBLISH";
        VLOG(0) << "=======";
        VLOG(0) << "Error: " << publish.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(publish.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(publish.getReserved());
        VLOG(0) << "RemainingLength: " << publish.getRemainingLength();
        VLOG(0) << "DUP: " << publish.getDup();
        VLOG(0) << "QoSLevel: " << static_cast<uint16_t>(publish.getQoSLevel());
        VLOG(0) << "Retain: " << publish.getRetain();
        VLOG(0) << "Topic: " << publish.getTopic();
        VLOG(0) << "PacketIdentifier: " << publish.getPacketIdentifier();
        VLOG(0) << "Message: " << publish.getMessage();

        switch (publish.getQoSLevel()) {
            case 1:
                sendPuback(publish.getPacketIdentifier());
                break;
            case 2:
                sendPubrec(publish.getPacketIdentifier());
                break;
        }

        broker->publish(publish.getTopic(), publish.getMessage(), publish.getRetain());
    }

    void SocketContext::onPuback(const iot::mqtt::packets::Puback& puback) {
        VLOG(0) << "PUBACK";
        VLOG(0) << "======";
        VLOG(0) << "Error: " << puback.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(puback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(puback.getReserved());
        VLOG(0) << "RemainingLength: " << puback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        VLOG(0) << "PUBREC";
        VLOG(0) << "======";
        VLOG(0) << "Error: " << pubrec.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(pubrec.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pubrec.getReserved());
        VLOG(0) << "RemainingLength: " << pubrec.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << pubrec.getPacketIdentifier();

        sendPubrel(pubrec.getPacketIdentifier());
    }

    void SocketContext::onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        VLOG(0) << "PUBREL";
        VLOG(0) << "======";
        VLOG(0) << "Error: " << pubrel.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(pubrel.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pubrel.getReserved());
        VLOG(0) << "RemainingLength: " << pubrel.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << pubrel.getPacketIdentifier();

        sendPubcomp(pubrel.getPacketIdentifier());
    }

    void SocketContext::onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        VLOG(0) << "PUBCOMP";
        VLOG(0) << "=======";
        VLOG(0) << "Error: " << pubcomp.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(pubcomp.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pubcomp.getReserved());
        VLOG(0) << "RemainingLength: " << pubcomp.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) {
        VLOG(0) << "SUBSCRIBE";
        VLOG(0) << "=========";
        VLOG(0) << "Error: " << subscribe.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(subscribe.getReserved());
        VLOG(0) << "RemainingLength: " << subscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        std::list<uint8_t> returnCodes;

        for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
            VLOG(0) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getRequestedQoS());
            broker->subscribe(topic.getName(), this, topic.getRequestedQoS());

            returnCodes.push_back(topic.getRequestedQoS() | 0x00 /* 0x80 */); // QoS + Success
        }

        sendSuback(subscribe.getPacketIdentifier(), returnCodes);

        subscribtionCount++;
    }

    void SocketContext::onSuback(const iot::mqtt::packets::Suback& suback) {
        VLOG(0) << "SUBACK";
        VLOG(0) << "======";
        VLOG(0) << "Error: " << suback.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(suback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(suback.getReserved());
        VLOG(0) << "RemainingLength: " << suback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << suback.getPacketIdentifier();

        for (uint8_t returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  Return Code: " << static_cast<uint16_t>(returnCode);
        }
    }

    void SocketContext::onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) {
        VLOG(0) << "UNSUBSCRIBE";
        VLOG(0) << "===========";
        VLOG(0) << "Error: " << unsubscribe.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsubscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsubscribe.getReserved());
        VLOG(0) << "RemainingLength: " << unsubscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            VLOG(0) << "  Topic: " << topic;
            broker->unsubscribe(topic, this);
        }

        sendUnsuback(unsubscribe.getPacketIdentifier());

        subscribtionCount--;
    }

    void SocketContext::onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) {
        VLOG(0) << "UNSUBACK";
        VLOG(0) << "========";
        VLOG(0) << "Error: " << unsuback.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsuback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsuback.getReserved());
        VLOG(0) << "RemainingLength: " << unsuback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsuback.getPacketIdentifier();
    }

    void SocketContext::onPingreq(const iot::mqtt::packets::Pingreq& pingreq) {
        VLOG(0) << "PINGREQ";
        VLOG(0) << "=======";
        VLOG(0) << "Error: " << pingreq.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingreq.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pingreq.getReserved());
        VLOG(0) << "RemainingLength: " << pingreq.getRemainingLength();

        sendPingresp();
    }

    void SocketContext::onPingresp(const iot::mqtt::packets::Pingresp& pingresp) {
        VLOG(0) << "PINGRESP";
        VLOG(0) << "========";
        VLOG(0) << "Error: " << pingresp.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingresp.getType());
        VLOG(0) << "RemainingLength: " << pingresp.getRemainingLength();
    }

    void SocketContext::onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) {
        VLOG(0) << "DISCONNECT";
        VLOG(0) << "==========";
        VLOG(0) << "Error: " << disconnect.isError();
        VLOG(0) << "Type: " << static_cast<uint16_t>(disconnect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(disconnect.getReserved());
        VLOG(0) << "RemainingLength: " << disconnect.getRemainingLength();

        close();
    }

} // namespace apps::mqtt::broker
