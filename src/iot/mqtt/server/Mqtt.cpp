/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "iot/mqtt/server/Mqtt.h"

#include "iot/mqtt/MqttContext.h"
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

#include <cstdint>
#include <iomanip>
#include <ios>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server {

    Mqtt::Mqtt(const std::string& connectionName, const std::shared_ptr<broker::Broker>& broker)
        : Super(connectionName)
        , broker(broker) {
    }

    Mqtt::~Mqtt() {
        releaseSession();

        if (willFlag) {
            broker->publish(clientId, willTopic, willMessage, willQoS, willRetain);
        }
    }

    void Mqtt::subscribe(const std::string& topic, uint8_t qoS) const {
        broker->subscribe(clientId, topic, qoS);
    }

    void Mqtt::unsubscribe(const std::string& topic) const {
        broker->unsubscribe(clientId, topic);
    }

    bool Mqtt::onSignal([[maybe_unused]] int sig) {
        willFlag = false;
        return true;
    }

    iot::mqtt::ControlPacketDeserializer* Mqtt::createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) {
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        switch (fixedHeader.getType()) {
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

    void Mqtt::deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) {
        static_cast<iot::mqtt::server::ControlPacketDeserializer*>(controlPacketDeserializer)->deliverPacket(this); // NOLINT
    }

    bool Mqtt::initSession(const utils::Timeval& keepAlive) {
        bool success = true;

        if (broker->hasActiveSession(clientId)) {
            LOG(ERROR) << connectionName << " MQTT Broker: Existing session found for ClientId = " << clientId;
            LOG(ERROR) << connectionName << " MQTT Broker:   closing";
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, 0);

            willFlag = false;
            success = false;
        } else if (broker->hasRetainedSession(clientId)) {
            LOG(INFO) << connectionName << " MQTT Broker: Retained session found for ClientId = " << clientId;
            if (cleanSession) {
                LOG(DEBUG) << connectionName << "   New SessionId = " << this;
                sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

                broker->unsubscribe(clientId);
                initSession(broker->newSession(clientId, this), keepAlive);
            } else {
                LOG(DEBUG) << connectionName << "   Renew SessionId = " << this;
                sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_PRESENT);

                initSession(broker->renewSession(clientId, this), keepAlive);
                broker->restartSession(clientId);
            }
        } else {
            LOG(INFO) << connectionName << " MQTT Broker: No session found for ClientId = " << clientId;
            LOG(INFO) << connectionName << " MQTT Broker:   new SessionId = " << this;

            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

            initSession(broker->newSession(clientId, this), keepAlive);
        }

        return success;
    }

    void Mqtt::releaseSession() {
        if (broker->isActiveSession(clientId, this)) {
            if (cleanSession) {
                LOG(DEBUG) << connectionName << " MQTT Broker: Delete session for ClientId = " << clientId;
                LOG(DEBUG) << connectionName << " MQTT Broker:   SessionId = " << this;
                broker->deleteSession(clientId);
            } else {
                LOG(DEBUG) << connectionName << " MQTT Broker: Retain session for ClientId = " << clientId;
                LOG(DEBUG) << connectionName << " MQTT Broker:   SessionId = " << this;
                broker->retainSession(clientId);
            }
        }
    }

    void Mqtt::onConnect([[maybe_unused]] const iot::mqtt::packets::Connect& connect) {
    }

    void Mqtt::onSubscribe([[maybe_unused]] const iot::mqtt::packets::Subscribe& subscribe) {
    }

    void Mqtt::onUnsubscribe([[maybe_unused]] const iot::mqtt::packets::Unsubscribe& unsubscribe) {
    }

    void Mqtt::onPingreq([[maybe_unused]] const iot::mqtt::packets::Pingreq& pingreq) {
    }

    void Mqtt::onDisconnect([[maybe_unused]] const iot::mqtt::packets::Disconnect& disconnect) {
    }

    void Mqtt::_onConnect(const iot::mqtt::server::packets::Connect& connect) {
        LOG(INFO) << connectionName << " MQTT Broker:   Protocol: " << connect.getProtocol();
        LOG(INFO) << connectionName << " MQTT Broker:   Version: " << static_cast<uint16_t>(connect.getLevel());
        LOG(INFO) << connectionName << " MQTT Broker:   ConnectFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<uint16_t>(connect.getConnectFlags()) << std::dec << std::setw(0);
        LOG(INFO) << connectionName << " MQTT Broker:   KeepAlive: " << connect.getKeepAlive();
        LOG(INFO) << connectionName << " MQTT Broker:   ClientID: " << connect.getClientId();
        LOG(INFO) << connectionName << " MQTT Broker:   CleanSession: " << connect.getCleanSession();

        if (connect.getWillFlag()) {
            LOG(INFO) << connectionName << " MQTT Broker:   WillTopic: " << connect.getWillTopic();
            LOG(INFO) << connectionName << " MQTT Broker:   WillMessage: " << connect.getWillMessage();
            LOG(INFO) << connectionName << " MQTT Broker:   WillQoS: " << static_cast<uint16_t>(connect.getWillQoS());
            LOG(INFO) << connectionName << " MQTT Broker:   WillRetain: " << connect.getWillRetain();
        }
        if (connect.getUsernameFlag()) {
            LOG(INFO) << connectionName << " MQTT Broker:   Username: " << connect.getUsername();
        }
        if (connect.getPasswordFlag()) {
            LOG(INFO) << connectionName << " MQTT Broker:   Password: " << connect.getPassword();
        }

        if (connect.getProtocol() != "MQTT") {
            LOG(ERROR) << connectionName << " MQTT Broker:   Wrong Protocol: " << connect.getProtocol();
            mqttContext->end(true);
        } else if ((connect.getLevel()) != MQTT_VERSION_3_1_1) {
            LOG(ERROR) << connectionName << " MQTT Broker:   Wrong Protocol Level: " << MQTT_VERSION_3_1_1 << " != " << connect.getLevel();
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);

            mqttContext->end(true);
        } else if (connect.isFakedClientId() && !connect.getCleanSession()) {
            LOG(ERROR) << connectionName << " MQTT Broker:   Resume session but no ClientId present";
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);

            mqttContext->end(true);
        } else {
            // V-Header
            protocol = connect.getProtocol();
            level = connect.getLevel();
            reflect = connect.getReflect();
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

            if (initSession(keepAlive)) {
                onConnect(connect);
            } else {
                mqttContext->end(true);
            }
        }
    }

    void Mqtt::_onPublish(const iot::mqtt::server::packets::Publish& publish) {
        if (Super::_onPublish(publish)) {
            broker->publish(clientId, publish.getTopic(), publish.getMessage(), publish.getQoS(), publish.getRetain());

            onPublish(publish);
        }
    }

    void Mqtt::_onSubscribe(const iot::mqtt::server::packets::Subscribe& subscribe) {
        if (subscribe.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT Broker:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT Broker:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << subscribe.getPacketIdentifier();

            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                LOG(INFO) << connectionName << " MQTT Broker:   Topic filter: '" << topic.getName()
                          << "', QoS: " << static_cast<uint16_t>(topic.getQoS());
            }

            std::list<uint8_t> returnCodes;
            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                const uint8_t returnCode = broker->subscribe(clientId, topic.getName(), topic.getQoS());
                returnCodes.push_back(returnCode);
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);

            onSubscribe(subscribe);
        }
    }

    void Mqtt::_onUnsubscribe(const iot::mqtt::server::packets::Unsubscribe& unsubscribe) {
        if (unsubscribe.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT Broker:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT Broker:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << unsubscribe.getPacketIdentifier();

            for (const std::string& topic : unsubscribe.getTopics()) {
                LOG(INFO) << connectionName << " MQTT Broker:   Topic: " << topic;
            }

            for (const std::string& topic : unsubscribe.getTopics()) {
                broker->unsubscribe(clientId, topic);
            }

            sendUnsuback(unsubscribe.getPacketIdentifier());

            onUnsubscribe(unsubscribe);
        }
    }

    void Mqtt::_onPingreq(const iot::mqtt::server::packets::Pingreq& pingreq) {
        sendPingresp();

        onPingreq(pingreq);
    }

    void Mqtt::_onDisconnect(const iot::mqtt::server::packets::Disconnect& disconnect) {
        willFlag = false;

        onDisconnect(disconnect);

        releaseSession();
    }

    void Mqtt::sendConnack(uint8_t returnCode, uint8_t flags) const { // Server
        send(iot::mqtt::packets::Connack(returnCode, flags));
    }

    void Mqtt::sendSuback(uint16_t packetIdentifier, const std::list<uint8_t>& returnCodes) const { // Server
        send(iot::mqtt::packets::Suback(packetIdentifier, returnCodes));
    }

    void Mqtt::sendUnsuback(uint16_t packetIdentifier) const { // Server
        send(iot::mqtt::packets::Unsuback(packetIdentifier));
    }

    void Mqtt::sendPingresp() const { // Server
        send(iot::mqtt::packets::Pingresp());
    }

    std::list<std::string> Mqtt::getSubscriptions() const {
        return broker->getSubscriptions(clientId);
    }

    std::string Mqtt::getProtocol() const {
        return protocol;
    }

    uint8_t Mqtt::getLevel() const {
        return level;
    }

    uint8_t Mqtt::getConnectFlags() const {
        return connectFlags;
    }

    uint16_t Mqtt::getKeepAlive() const {
        return keepAlive;
    }

    std::string Mqtt::getClientId() const {
        return clientId;
    }

    std::string Mqtt::getWillTopic() const {
        return willTopic;
    }

    std::string Mqtt::getWillMessage() const {
        return willMessage;
    }

    std::string Mqtt::getUsername() const {
        return username;
    }

    std::string Mqtt::getPassword() const {
        return password;
    }

    bool Mqtt::getUsernameFlag() const {
        return usernameFlag;
    }

    bool Mqtt::getPasswordFlag() const {
        return passwordFlag;
    }

    bool Mqtt::getWillRetain() const {
        return willRetain;
    }

    uint8_t Mqtt::getWillQoS() const {
        return willQoS;
    }

    bool Mqtt::getWillFlag() const {
        return willFlag;
    }

    bool Mqtt::getCleanSession() const {
        return cleanSession;
    }

    bool Mqtt::getReflect() const {
        return reflect;
    }

} // namespace iot::mqtt::server
