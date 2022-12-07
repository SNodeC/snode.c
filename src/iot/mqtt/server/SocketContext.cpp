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
#include "iot/mqtt/server/packets/Connect.h"
#include "iot/mqtt/server/packets/Disconnect.h"
#include "iot/mqtt/server/packets/Pingreq.h"
#include "iot/mqtt/server/packets/Puback.h"
#include "iot/mqtt/server/packets/Pubcomp.h"
#include "iot/mqtt/server/packets/Publish.h"
#include "iot/mqtt/server/packets/Pubrec.h"
#include "iot/mqtt/server/packets/Pubrel.h"
#include "iot/mqtt/server/packets/Subscribe.h"
#include "iot/mqtt/server/packets/Unsubscribe.h"

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
            broker->publish(willTopic, willMessage, willQoS);

            if (willRetain) {
                broker->retainMessage(willTopic, willMessage, willQoS);
            }
        }
    }

    iot::mqtt::ControlPacketDeserializer* SocketContext::createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) {
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        switch (fixedHeader.getPacketType()) {
            case MQTT_CONNECT:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Connect(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBLISH:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Publish(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBACK:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Puback(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREC:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Pubrec(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREL:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Pubrel(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBCOMP:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Pubcomp(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_SUBSCRIBE:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Subscribe(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_UNSUBSCRIBE:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Unsubscribe(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PINGREQ:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Pingreq(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_DISCONNECT:
                controlPacketDeserializer =
                    new iot::mqtt::server::packets::Disconnect(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            default:
                controlPacketDeserializer = nullptr;
                break;
        }

        return controlPacketDeserializer;
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
            sendConnack(MQTT_CONNACK_ACCEPT, cleanSession ? MQTT_SESSION_NEW : MQTT_SESSION_PRESENT);

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
        if (broker->isActiveSession(clientId, this)) {
            if (cleanSession) {
                LOG(DEBUG) << "Delete session: " << clientId;
                broker->deleteSession(clientId);
            } else {
                LOG(DEBUG) << "Retain session: " << clientId;
                broker->retainSession(clientId);
            }
        }
    }

    void SocketContext::onConnect([[maybe_unused]] iot::mqtt::packets::Connect& connect) {
    }

    void SocketContext::onPublish([[maybe_unused]] iot::mqtt::packets::Publish& publish) {
    }

    void SocketContext::onPuback([[maybe_unused]] iot::mqtt::packets::Puback& puback) {
    }

    void SocketContext::onPubrec([[maybe_unused]] iot::mqtt::packets::Pubrec& pubrec) {
    }

    void SocketContext::onPubrel([[maybe_unused]] iot::mqtt::packets::Pubrel& pubrel) {
    }

    void SocketContext::onPubcomp([[maybe_unused]] iot::mqtt::packets::Pubcomp& pubcomp) {
    }

    void SocketContext::onSubscribe([[maybe_unused]] iot::mqtt::packets::Subscribe& subscribe) {
    }

    void SocketContext::onUnsubscribe([[maybe_unused]] iot::mqtt::packets::Unsubscribe& unsubscribe) {
    }

    void SocketContext::onPingreq([[maybe_unused]] iot::mqtt::packets::Pingreq& pingreq) {
    }

    void SocketContext::onDisconnect([[maybe_unused]] iot::mqtt::packets::Disconnect& disconnect) {
    }

    void SocketContext::_onConnect(iot::mqtt::server::packets::Connect& connect) {
        LOG(DEBUG) << "Received CONNECT: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(connect);
        LOG(DEBUG) << "Protocol: " << connect.getProtocol();
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(connect.getLevel());
        LOG(DEBUG) << "ConnectFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(connect.getConnectFlags()) << std::dec << std::setw(0);
        LOG(DEBUG) << "KeepAlive: " << connect.getKeepAlive();
        LOG(DEBUG) << "ClientID: " << connect.getClientId();
        LOG(DEBUG) << "CleanSession: " << connect.getCleanSession();

        if (connect.getWillFlag()) {
            LOG(DEBUG) << "WillTopic: " << connect.getWillTopic();
            LOG(DEBUG) << "WillMessage: " << connect.getWillMessage();
            LOG(DEBUG) << "WillQoS: " << static_cast<uint16_t>(connect.getWillQoS());
            LOG(DEBUG) << "WillRetain: " << connect.getWillRetain();
        }
        if (connect.getUsernameFlag()) {
            LOG(DEBUG) << "Username: " << connect.getUsername();
        }
        if (connect.getPasswordFlag()) {
            LOG(DEBUG) << "Password: " << connect.getPassword();
        }

        if (connect.getProtocol() != "MQTT") {
            shutdown(true);
        } else if (connect.getLevel() != MQTT_VERSION_3_1_1) {
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);

            shutdown(true);
        } else if (connect.getClientId() == "" && !connect.getCleanSession()) {
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);

            shutdown(true);
        } else {
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

            if (keepAlive != 0) {
                setTimeout(1.5 * keepAlive);
            } else {
                setTimeout(core::DescriptorEventReceiver::TIMEOUT::DISABLE); // Or leaf at framework default (default 60 sec)?
            }

            initSession();

            onConnect(connect);
        }
    }

    void SocketContext::_onPublish(iot::mqtt::server::packets::Publish& publish) {
        LOG(DEBUG) << "Received PUBLISH: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(publish);
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "Message: " << publish.getMessage();
        LOG(DEBUG) << "QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "Retain: " << publish.getRetain();

        if (publish.getQoS() > 2) {
            shutdown(true);
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoS() > 0) {
            shutdown(true);
        } else {
            broker->publish(publish.getTopic(), publish.getMessage(), publish.getQoS());
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

            onPublish(publish);
        }
    }

    void SocketContext::_onPuback(iot::mqtt::server::packets::Puback& puback) {
        LOG(DEBUG) << "Received PUBACK: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(puback);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << puback.getPacketIdentifier();

        if (puback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            broker->pubackReceived(clientId, puback.getPacketIdentifier());

            onPuback(puback);
        }
    }

    void SocketContext::_onPubrec(iot::mqtt::server::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "Received PUBREC: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(pubrec);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrec.getPacketIdentifier();

        if (pubrec.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            broker->pubrecReceived(clientId, pubrec.getPacketIdentifier());

            sendPubrel(pubrec.getPacketIdentifier());

            onPubrec(pubrec);
        }
    }

    void SocketContext::_onPubrel(iot::mqtt::server::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "Received PUBREL: " << clientId;
        LOG(DEBUG) << "================";
        printStandardHeader(pubrel);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrel.getPacketIdentifier();

        if (pubrel.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            broker->pubrelReceived(clientId, pubrel.getPacketIdentifier());

            sendPubcomp(pubrel.getPacketIdentifier());

            onPubrel(pubrel);
        }
    }

    void SocketContext::_onPubcomp(iot::mqtt::server::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "Received PUBCOMP: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(pubcomp);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubcomp.getPacketIdentifier();

        if (pubcomp.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            broker->pubcompReceived(clientId, pubcomp.getPacketIdentifier());

            onPubcomp(pubcomp);
        }
    }

    void SocketContext::_onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "Received SUBSCRIBE: " << clientId;
        LOG(DEBUG) << "===================";
        printStandardHeader(subscribe);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << subscribe.getPacketIdentifier();

        for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic filter: '" << topic.getName() << "', QoS: " << static_cast<uint16_t>(topic.getQoS());
        }

        if (subscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            std::list<uint8_t> returnCodes;
            for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
                uint8_t returnCode = broker->subscribeReceived(clientId, topic.getName(), topic.getQoS());
                returnCodes.push_back(returnCode);
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);

            onSubscribe(subscribe);
        }
    }

    void SocketContext::_onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "Received UNSUBSCRIBE: " << clientId;
        LOG(DEBUG) << "=====================";
        printStandardHeader(unsubscribe);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
        }

        if (unsubscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            for (const std::string& topic : unsubscribe.getTopics()) {
                broker->unsubscribeReceived(clientId, topic);
            }

            sendUnsuback(unsubscribe.getPacketIdentifier());

            onUnsubscribe(unsubscribe);
        }
    }

    void SocketContext::_onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "Received PINGREQ: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(pingreq);

        sendPingresp();

        onPingreq(pingreq);
    }

    void SocketContext::_onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "Received DISCONNECT: " << clientId;
        LOG(DEBUG) << "====================";
        printStandardHeader(disconnect);

        willFlag = false;

        onDisconnect(disconnect);

        releaseSession();

        shutdown();
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) const {
        LOG(DEBUG) << "Send CONNACK";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Connack(returnCode, flags));
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) const {
        LOG(DEBUG) << "Send SUBACK";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) const {
        LOG(DEBUG) << "Send UNSUBACK";
        LOG(DEBUG) << "=============";

        send(iot::mqtt::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingresp() const { // Server
        LOG(DEBUG) << "Send Pingresp";
        LOG(DEBUG) << "=============";

        send(iot::mqtt::packets::Pingresp());
    }

    std::string SocketContext::getProtocol() const {
        return protocol;
    }

    uint8_t SocketContext::getLevel() const {
        return level;
    }

    uint8_t SocketContext::getConnectFlags() const {
        return connectFlags;
    }

    uint16_t SocketContext::getKeepAlive() const {
        return keepAlive;
    }

    std::string SocketContext::getClientId() const {
        return clientId;
    }

    std::string SocketContext::getWillTopic() const {
        return willTopic;
    }

    std::string SocketContext::getWillMessage() const {
        return willMessage;
    }

    std::string SocketContext::getUsername() const {
        return username;
    }

    std::string SocketContext::getPassword() const {
        return password;
    }

    bool SocketContext::getUsernameFlag() const {
        return usernameFlag;
    }

    bool SocketContext::getPasswordFlag() const {
        return passwordFlag;
    }

    bool SocketContext::getWillRetain() const {
        return willRetain;
    }

    uint8_t SocketContext::getWillQoS() const {
        return willQoS;
    }

    bool SocketContext::getWillFlag() const {
        return willFlag;
    }

    bool SocketContext::getCleanSession() const {
        return cleanSession;
    }

} // namespace iot::mqtt::server
