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

#include "core/DescriptorEventReceiver.h"
#include "iot/mqtt/packets/Connack.h"
#include "iot/mqtt/packets/Pingresp.h"
#include "iot/mqtt/packets/Suback.h"
#include "iot/mqtt/packets/Unsuback.h"
#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h" // IWYU pragma: keep

#include <cstdint>
#include <iomanip>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, const std::shared_ptr<broker::Broker>& broker)
        : iot::mqtt::SocketContext(socketConnection)
        , broker(broker) {
    }

    SocketContext::~SocketContext() {
        releaseSession();

        if (willFlag) {
            broker->publish(willTopic, willMessage, MQTT_DUP_FALSE, willQoS);

            if (willRetain) {
                broker->retainMessage(willTopic, willMessage, willQoS);
            }
        }
    }

    iot::mqtt::ControlPacketDeserializer* SocketContext::onReceiveFromPeer(iot::mqtt::FixedHeader& fixedHeader) {
        iot::mqtt::ControlPacketDeserializer* currentPacket = nullptr;

        switch (fixedHeader.getPacketType()) {
            case MQTT_CONNECT:
                currentPacket = new iot::mqtt::server::packets::Connect(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBLISH:
                currentPacket = new iot::mqtt::server::packets::Publish(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBACK:
                currentPacket = new iot::mqtt::server::packets::Puback(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREC:
                currentPacket = new iot::mqtt::server::packets::Pubrec(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREL:
                currentPacket = new iot::mqtt::server::packets::Pubrel(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBCOMP:
                currentPacket = new iot::mqtt::server::packets::Pubcomp(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_SUBSCRIBE:
                currentPacket = new iot::mqtt::server::packets::Subscribe(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_UNSUBSCRIBE:
                currentPacket = new iot::mqtt::server::packets::Unsubscribe(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PINGREQ:
                currentPacket = new iot::mqtt::server::packets::Pingreq(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_DISCONNECT:
                currentPacket = new iot::mqtt::server::packets::Disconnect(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            default:
                currentPacket = nullptr;
                break;
        }

        return currentPacket;
    }

    void SocketContext::propagateEvent(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) {
        dynamic_cast<iot::mqtt::server::ControlPacketDeserializer*>(controlPacketDeserializer)->propagateEvent(this);
    }

    void SocketContext::initSession() {
        if (broker->hasActiveSession(clientId)) {
            LOG(TRACE) << "Existing session found for ClientId = `" << clientId << "'";
            LOG(TRACE) << "  closing";

            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, 0);
            shutdown(true);
        } else if (broker->hasRetainedSession(clientId)) {
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_PRESENT);

            LOG(TRACE) << "Retained session found for ClientId = '" << clientId << "'";
            if (cleanSession) {
                LOG(TRACE) << "  clean Session = " << this;
                broker->unsubscribe(clientId);
                broker->newSession(clientId, this);
            } else {
                LOG(TRACE) << "  renew Session = " << this;
                broker->renewSession(clientId, this);
            }
        } else {
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

            LOG(TRACE) << "No session found for ClientId = '" << clientId << "\'";
            LOG(TRACE) << "  new Session = " << this;

            broker->newSession(clientId, this);
        }
    }

    void SocketContext::releaseSession() {
        if (broker->isActiveSesscion(clientId, this)) {
            if (cleanSession) {
                LOG(DEBUG) << "Delete session: " << clientId;
                broker->deleteSession(clientId);
            } else {
                LOG(DEBUG) << "Retain session: " << clientId;
                broker->retainSession(clientId);
            }
        }
    }

    void SocketContext::onConnect(iot::mqtt::packets::Connect& connect) {
        LOG(DEBUG) << "Received CONNECT: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(connect);
        LOG(DEBUG) << "Protocol: " << protocol;
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(level);
        LOG(DEBUG) << "ConnectFlags: " << static_cast<uint16_t>(connectFlags);
        LOG(DEBUG) << "KeepAlive: " << keepAlive;
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
        LOG(DEBUG) << "Received PUBLISH: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(publish);
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();
    }

    void SocketContext::onPuback(iot::mqtt::packets::Puback& puback) {
        LOG(DEBUG) << "Received PUBACK: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(puback);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(iot::mqtt::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "Received PUBREC: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(pubrec);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrec.getPacketIdentifier();
    }

    void SocketContext::onPubrel(iot::mqtt::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "Received PUBREL: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(pubrel);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrel.getPacketIdentifier();
    }

    void SocketContext::onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "Received PUBCOMP: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(pubcomp);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(iot::mqtt::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "Received SUBSCRIBE: " << clientId;
        LOG(DEBUG) << "===================";
        printStandardHeader(subscribe);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << subscribe.getPacketIdentifier();

        for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getQoS());
        }
    }

    void SocketContext::onUnsubscribe(iot::mqtt::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "Received UNSUBSCRIBE: " << clientId;
        LOG(DEBUG) << "=====================";
        printStandardHeader(unsubscribe);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
        }
    }

    void SocketContext::onPingreq(iot::mqtt::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "Received PINGREQ: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(pingreq);
    }

    void SocketContext::onDisconnect(iot::mqtt::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "Received DISCONNECT: " << clientId;
        LOG(DEBUG) << "====================";
        printStandardHeader(disconnect);
    }

    void SocketContext::_onConnect(iot::mqtt::server::packets::Connect& connect) {
        if (connect.getProtocol() != "MQTT") {
            shutdown(true);
        } else if (connect.getLevel() != MQTT_VERSION_3_1_1) {
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);
            shutdown(true);

        } else if (connect.getEffectiveClientId() == "" && !connect.getCleanSession()) {
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);
            shutdown(true);
        } else {
            if (connect.getEffectiveClientId().empty()) {
                connect.setEffectiveClientId(getRandomClientId());
            }

            // V-Header
            protocol = connect.getProtocol();
            level = connect.getLevel();
            connectFlags = connect.getConnectFlags();
            keepAlive = connect.getKeepAlive();

            // Payload
            clientId = connect.getEffectiveClientId();
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

            if (keepAlive != 0) {
                setTimeout(1.5 * keepAlive);
            } else {
                setTimeout(core::DescriptorEventReceiver::TIMEOUT::DISABLE); // Or leaf at framework default (default 60 sec)?
            }

            onConnect(connect);

            initSession();
        }
    }

    void SocketContext::_onPublish(iot::mqtt::server::packets::Publish& publish) {
        if (publish.getQoS() > 2) {
            shutdown(true);
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoS() > 0) {
            shutdown(true);
        } else {
            onPublish(publish);

            broker->publish(publish.getTopic(), publish.getMessage(), publish.getDup(), publish.getQoS());
            if (publish.getRetain()) {
                broker->retainMessage(publish.getTopic(), publish.getMessage(), publish.getQoS());
            }

            switch (publish.getQoS()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());
                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());
                    break;
            }
        }
    }

    void SocketContext::_onPuback(iot::mqtt::server::packets::Puback& puback) {
        if (puback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPuback(puback);

            broker->pubackReceived(puback.getPacketIdentifier(), clientId);
        }
    }

    void SocketContext::_onPubrec(iot::mqtt::server::packets::Pubrec& pubrec) {
        if (pubrec.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubrec(pubrec);

            broker->pubrecReceived(pubrec.getPacketIdentifier(), clientId);

            sendPubrel(pubrec.getPacketIdentifier());
        }
    }

    void SocketContext::_onPubrel(iot::mqtt::server::packets::Pubrel& pubrel) {
        if (pubrel.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubrel(pubrel);

            broker->pubrelReceived(pubrel.getPacketIdentifier(), clientId);

            sendPubcomp(pubrel.getPacketIdentifier());
        }
    }

    void SocketContext::_onPubcomp(iot::mqtt::server::packets::Pubcomp& pubcomp) {
        if (pubcomp.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubcomp(pubcomp);

            broker->pubcompReceived(pubcomp.getPacketIdentifier(), clientId);
        }
    }

    void SocketContext::_onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) {
        if (subscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSubscribe(subscribe);

            std::list<uint8_t> returnCodes;
            for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
                uint8_t returnCode = broker->subscribeReceived(clientId, topic.getName(), topic.getQoS());
                returnCodes.push_back(returnCode);
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);
        }
    }

    void SocketContext::_onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) {
        if (unsubscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onUnsubscribe(unsubscribe);

            for (const std::string& topic : unsubscribe.getTopics()) {
                broker->unsubscribeReceived(clientId, topic);
            }

            sendUnsuback(unsubscribe.getPacketIdentifier());
        }
    }

    void SocketContext::_onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) {
        onPingreq(pingreq);

        sendPingresp();
    }

    void SocketContext::_onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) {
        willFlag = false;

        onDisconnect(disconnect);

        releaseSession();

        shutdown();
    }

    void SocketContext::publish(const std::string& topic, const std::string& message, uint8_t qoS) {
        broker->publish(topic, message, MQTT_DUP_FALSE, qoS);
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) {
        LOG(DEBUG) << "Send CONNACK";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Connack(returnCode, flags));

        if (returnCode != MQTT_CONNACK_ACCEPT) {
            shutdown(true);
        }
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        LOG(DEBUG) << "Send SUBACK";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        LOG(DEBUG) << "Send UNSUBACK";
        LOG(DEBUG) << "=============";

        send(iot::mqtt::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingresp() { // Server
        LOG(DEBUG) << "Send Pingresp";
        LOG(DEBUG) << "=============";

        send(iot::mqtt::packets::Pingresp());
    }

} // namespace iot::mqtt::server
