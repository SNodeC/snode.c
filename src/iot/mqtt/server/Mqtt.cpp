/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/SemanticLog.h"
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

#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"

#include <cstdint>
#include <iomanip>
#include <optional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server {
    namespace {
        logger::BoundaryLogger serverLog(const Mqtt& mqtt) {
            const iot::mqtt::MqttContext* context = mqtt.getMqttContext();
            if (context != nullptr) {
                const core::socket::stream::SocketConnection* connection = context->getSocketConnection();
                if (connection != nullptr) {
                    return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                                 logger::LogBoundary::Connection,
                                                 "iot.mqtt.server",
                                                 connection->getInstanceName().empty()
                                                     ? std::nullopt
                                                     : std::optional<std::string>(connection->getInstanceName()),
                                                 logger::LogRole::Server,
                                                 std::to_string(connection->getConnectionId()))
                        .logger(logger::Logger::semanticSink());
                }
            }
            return iot::mqtt::semantic::mqttServerLog();
        }
    } // namespace

    Mqtt::Mqtt(const std::string& connectionName, const std::shared_ptr<broker::Broker>& broker)
        : Super(connectionName)
        , broker(broker) {
    }

    Mqtt::~Mqtt() {
        releaseSession(iot::mqtt::semantic::mqttServerLog(), connectionName + ": ");

        if (willFlag) {
            broker->publish(clientId, willTopic, willMessage, willQoS, willRetain);
        }
    }

    bool Mqtt::onSignal([[maybe_unused]] int sig) {
        willFlag = false;
        return true;
    }

    iot::mqtt::ControlPacketDeserializer* Mqtt::createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) const {
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr; // NOLINT

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
        const auto log = serverLog(*this);

        if (broker->hasActiveSession(clientId)) {
            log.error() << "Existing session found for ClientId = " << clientId;
            log.error() << "closing";
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, 0);

            willFlag = false;
            sessionRejected(clientId);
            success = false;
        } else if (broker->hasRetainedSession(clientId)) {
            log.info() << "Retained session found for ClientId = " << clientId;
            if (cleanSession) {
                log.debug() << "New SessionId = " << this;
                sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

                broker->unsubscribe(clientId);
                initSession(broker->newSession(clientId, this), keepAlive);
                sessionEstablished(false);
            } else {
                log.debug() << "Renew SessionId = " << this;
                sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_PRESENT);

                initSession(broker->renewSession(clientId, this), keepAlive);
                broker->restartSession(clientId);
                sessionEstablished(true);
            }
        } else {
            log.info() << "No session found for ClientId = " << clientId;
            log.info() << "new SessionId = " << this;

            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

            initSession(broker->newSession(clientId, this), keepAlive);
            sessionEstablished(false);
        }

        return success;
    }

    void Mqtt::releaseSession(logger::BoundaryLogger log, std::string_view prefix) {
        if (broker->isActiveSession(clientId, this)) {
            if (cleanSession) {
                log.debug() << prefix << "Delete session for ClientId = " << clientId;
                log.debug() << prefix << "SessionId = " << this;
                broker->deleteSession(clientId);
            } else {
                log.debug() << prefix << "Retain session for ClientId = " << clientId;
                log.debug() << prefix << "SessionId = " << this;
                broker->retainSession(clientId);
            }
        }
    }

    void Mqtt::distributePublish(const iot::mqtt::packets::Publish& publish) {
        broker->publish(clientId, publish.getTopic(), publish.getMessage(), publish.getQoS(), publish.getRetain());

        onPublish(publish);
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
        const auto log = serverLog(*this);
        log.info() << "Protocol: " << connect.getProtocol();
        log.info() << "Version: " << static_cast<uint16_t>(connect.getLevel());
        log.info() << "ConnectFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(connect.getConnectFlags()) << std::dec << std::setw(0);
        log.info() << "KeepAlive: " << connect.getKeepAlive();
        log.info() << "ClientID: " << connect.getClientId();
        log.info() << "CleanSession: " << connect.getCleanSession();

        if (connect.getWillFlag()) {
            log.debug() << "WillTopic: " << connect.getWillTopic();
            log.debug() << "WillMessage supplied: " << !connect.getWillMessage().empty() << " (size=" << connect.getWillMessage().size()
                        << ")";
            log.info() << "WillQoS: " << static_cast<uint16_t>(connect.getWillQoS());
            log.info() << "WillRetain: " << connect.getWillRetain();
        }
        log.debug() << "Username supplied: " << connect.getUsernameFlag();
        log.debug() << "Password supplied: " << connect.getPasswordFlag();

        if (connect.getProtocol() != "MQTT") {
            log.error() << "Wrong Protocol: " << connect.getProtocol();
            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if ((connect.getLevel()) != MQTT_VERSION_3_1_1) {
            log.error() << "Wrong Protocol Level: " << MQTT_VERSION_3_1_1 << " != " << connect.getLevel();
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);

            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if ((connect.getConnectFlags() & 0x01) != 0) {
            log.error() << "CONNECT reserved flag bit set";

            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if (connect.isFakedClientId() && !connect.getCleanSession()) {
            log.error() << "Resume session but no ClientId present";
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);

            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if (!connect.getWillFlag() && (connect.getWillQoS() != 0 || connect.getWillRetain())) {
            log.error() << "WillFlag not set but WillQoS or WillRetain set";

            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if (connect.getWillQoS() > 2) {
            log.error() << "WillQoS larger than 2";

            sessionRejected(connect.getClientId());
            mqttContext->close();
        } else if (connect.getPasswordFlag() && !connect.getUsernameFlag()) {
            log.error() << "Password flag set but username flag not";

            sessionRejected(connect.getClientId());
            mqttContext->close();
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
                mqttContext->close();
            }
        }
    }

    void Mqtt::_onPublish(const iot::mqtt::server::packets::Publish& publish) {
        if (Super::_onPublish(publish)) {
            distributePublish(publish);
        }
    }

    void Mqtt::_onSubscribe(const iot::mqtt::server::packets::Subscribe& subscribe) {
        const auto log = serverLog(*this);
        if (subscribe.getPacketIdentifier() == 0) {
            log.error() << "PackageIdentifier missing";
            mqttContext->close();
        } else {
            log.debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << subscribe.getPacketIdentifier()
                        << std::dec;

            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                log.info() << "Topic filter: '" << topic.getName() << "', QoS: " << static_cast<uint16_t>(topic.getQoS());
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
        const auto log = serverLog(*this);
        if (unsubscribe.getPacketIdentifier() == 0) {
            log.error() << "PackageIdentifier missing";
            mqttContext->close();
        } else {
            log.debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << unsubscribe.getPacketIdentifier()
                        << std::dec;

            for (const std::string& topic : unsubscribe.getTopics()) {
                log.info() << "Topic: " << topic;
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

        releaseSession(serverLog(*this), {});
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
