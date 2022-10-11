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

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h" // IWYU pragma: keep

#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, const std::shared_ptr<broker::Broker>& broker)
        : iot::mqtt::SocketContext(socketConnection)
        , broker(broker) {
    }

    SocketContext::~SocketContext() {
        releaseSession();

        if (willFlag) {
            broker->publish(willTopic, willMessage, willQoS, willRetain);
        }
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

    void SocketContext::initSession() {
        if (broker->hasActiveSession(clientId)) {
            LOG(TRACE) << "ClientID \'" << clientId << "\' already in use ... disconnecting";

            close();
        } else if (broker->hasRetainedSession(clientId)) {
            LOG(TRACE) << "ClientId \'" << clientId << "\' found retained session ... renewing";
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_PRESENT);

            if (cleanSession) {
                LOG(TRACE) << "CleanSession ... discarding subscribtions";
                broker->unsubscribe(clientId);
            } else {
                LOG(TRACE) << "RetainSession ... retaining subscribtions";
            }

            broker->renewSession(clientId, this);
        } else {
            LOG(TRACE) << "ClientId \'" << clientId << "\' no existing session ... creating";
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

            broker->newSession(clientId, this);
        }
    }

    void SocketContext::releaseSession() {
        if (cleanSession) {
            broker->deleteSession(clientId, this);
        } else {
            broker->retainSession(clientId, this);
        }
    }

    void SocketContext::onConnect(iot::mqtt::server::packets::Connect& connect) {
        LOG(DEBUG) << "CONNECT";
        LOG(DEBUG) << "=======";
        printStandardHeader(connect);
        LOG(DEBUG) << "Protocol: " << protocol;
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(level);
        LOG(DEBUG) << "ConnectFlags: " << static_cast<uint16_t>(connectFlags);
        LOG(DEBUG) << "KeepAlive: " << keepAlive;
        if (keepAlive != 0) {
            setTimeout(1.5 * keepAlive);
        } else {
            // setTimeout(core::DescriptorEventReceiver::TIMEOUT::DISABLE); // Leaf at framework default (default 60 sec)
        }
        LOG(DEBUG) << "ClientID: " << clientId;
        LOG(DEBUG) << "CleanSession: " << cleanSession;
        if (willFlag) {
            LOG(DEBUG) << "WillTopic: " << willTopic;
            LOG(DEBUG) << "WillMessage: " << willMessage;
            LOG(DEBUG) << "WillQoS: " << static_cast<uint16_t>(willQoS);
            LOG(DEBUG) << "WillRetain: " << willRetain;
        }
        if (usernameFlag) {
            LOG(DEBUG) << "Username: " << username;
        }
        if (passwordFlag) {
            LOG(DEBUG) << "Password: " << password;
        }
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "PUBLISH";
        LOG(DEBUG) << "=======";
        printStandardHeader(publish);
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoSLevel: " << static_cast<uint16_t>(publish.getQoSLevel());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();
    }

    void SocketContext::onPuback(iot::mqtt::packets::Puback& puback) {
        LOG(DEBUG) << "PUBACK";
        LOG(DEBUG) << "======";
        printStandardHeader(puback);
        LOG(DEBUG) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(iot::mqtt::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "PUBREC";
        LOG(DEBUG) << "======";
        printStandardHeader(pubrec);
        LOG(DEBUG) << "PacketIdentifier: " << pubrec.getPacketIdentifier();
    }

    void SocketContext::onPubrel(iot::mqtt::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "PUBREL";
        LOG(DEBUG) << "======";
        printStandardHeader(pubrel);
        LOG(DEBUG) << "PacketIdentifier: " << pubrel.getPacketIdentifier();
    }

    void SocketContext::onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "PUBCOMP";
        LOG(DEBUG) << "=======";
        printStandardHeader(pubcomp);
        LOG(DEBUG) << "PacketIdentifier: " << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "SUBSCRIBE";
        LOG(DEBUG) << "=========";
        printStandardHeader(subscribe);
        LOG(DEBUG) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getRequestedQoS());
        }
    }

    void SocketContext::onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "UNSUBSCRIBE";
        LOG(DEBUG) << "===========";
        printStandardHeader(unsubscribe);
        LOG(DEBUG) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
        }
    }

    void SocketContext::onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "PINGREQ";
        LOG(DEBUG) << "=======";
        printStandardHeader(pingreq);
    }

    void SocketContext::onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "DISCONNECT";
        LOG(DEBUG) << "==========";
        printStandardHeader(disconnect);
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

            // V-Header
            protocol = connect.getProtocol();
            level = connect.getLevel();
            connectFlags = connect.getConnectFlags();
            keepAlive = connect.getKeepAlive();

            // Payload
            clientId = connect.getClientId();
            willTopic = connect.getWillTopic();
            willMessage = connect.getWillMessage();
            username = connect.getUsername();
            password = connect.getPassword();

            // Derived from flags
            usernameFlag = connect.getUsernameFlag();
            passwordFlag = connect.getPasswordFlag();
            willRetain = connect.getWillRetain();
            willQoS = connect.getWillQoS();
            willFlag = connect.getWillFlag();
            cleanSession = connect.getCleanSession();

            initSession();

            onConnect(connect);
        }
    }

    void SocketContext::__onPublish(mqtt::packets::Publish& publish) {
        broker->publish(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
        if (publish.getRetain()) {
            broker->retain(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
        }

        onPublish(publish);
    }

    void SocketContext::_onSubscribe(packets::Subscribe& subscribe) {
        if (subscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
                uint8_t ret = broker->subscribe(topic.getName(), clientId, topic.getRequestedQoS());
                topic.setAcceptedQoS(ret);
            }

            std::list<uint8_t> returnCodes;

            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                returnCodes.push_back(topic.getAcceptedQoS());
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);

            onSubscribe(subscribe);
        }
    }

    void SocketContext::_onUnsubscribe(packets::Unsubscribe& unsubscribe) {
        if (unsubscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onUnsubscribe(unsubscribe);

            for (const std::string& topic : unsubscribe.getTopics()) {
                broker->unsubscribe(topic, clientId);
            }

            sendUnsuback(unsubscribe.getPacketIdentifier());
        }
    }

    void SocketContext::_onPingreq(packets::Pingreq& pingreq) {
        sendPingresp();

        onPingreq(pingreq);
    }

    void SocketContext::_onDisconnect(packets::Disconnect& disconnect) {
        willFlag = false;

        onDisconnect(disconnect);

        releaseSession();

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
