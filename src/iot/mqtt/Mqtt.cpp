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

#include "Mqtt.h"

#include "MqttContext.h"
#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/Session.h"
#include "iot/mqtt/packets/Puback.h"
#include "iot/mqtt/packets/Pubcomp.h"
#include "iot/mqtt/packets/Publish.h"
#include "iot/mqtt/packets/Pubrec.h"
#include "iot/mqtt/packets/Pubrel.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/hexdump.h"

#include <functional>
#include <iomanip>
#include <ios>
#include <map>
#include <set>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    Mqtt::Mqtt(const std::string& connectionName)
        : connectionName(connectionName) {
    }

    Mqtt::Mqtt(const std::string& connectionName, const std::string& clientId)
        : connectionName(connectionName)
        , clientId(clientId) {
    }

    Mqtt::~Mqtt() {
        if (controlPacketDeserializer != nullptr) {
            delete controlPacketDeserializer;
            controlPacketDeserializer = nullptr;
        }

        keepAliveTimer.cancel();
    }

    void Mqtt::setMqttContext(MqttContext* mqttContext) {
        this->mqttContext = mqttContext;
    }

    const MqttContext* Mqtt::getMqttContext() const {
        return mqttContext;
    }

    void Mqtt::onConnected() {
        LOG(INFO) << "MQTT: Connected";
    }

    std::size_t Mqtt::onReceivedFromPeer() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = fixedHeader.deserialize(mqttContext);

                if (!fixedHeader.isComplete()) {
                    break;
                }
                if (fixedHeader.isError()) {
                    mqttContext->close();
                    break;
                }
                printFixedHeader(fixedHeader);

                controlPacketDeserializer = createControlPacketDeserializer(fixedHeader);

                fixedHeader.reset();

                if (controlPacketDeserializer == nullptr) {
                    LOG(DEBUG) << connectionName << " MQTT: Received packet-type is unavailable ... closing connection";

                    mqttContext->end(true);
                    break;
                }
                if (controlPacketDeserializer->isError()) {
                    LOG(DEBUG) << connectionName << " MQTT: Fixed header has error ... closing connection";

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    mqttContext->end(true);
                    break;
                }

                state++;

                [[fallthrough]];
            case 1:
                consumed += controlPacketDeserializer->deserialize(mqttContext);

                if (controlPacketDeserializer->isError()) {
                    LOG(DEBUG) << connectionName << " MQTT: Control packet has error ... closing connection";
                    mqttContext->end(true);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                } else if (controlPacketDeserializer->isComplete()) {
                    deliverPacket(controlPacketDeserializer);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;

                    keepAliveTimer.restart();
                }

                break;
        }

        return consumed;
    }

    void Mqtt::onDisconnected() {
        LOG(INFO) << connectionName << " MQTT: Disconnected";
    }

    const std::string& Mqtt::getConnectionName() const {
        return connectionName;
    }

    void Mqtt::initSession(Session* session, utils::Timeval keepAlive) {
        this->session = session;

        for (const auto& [packetIdentifier, publish] : session->publishMap) {
            LOG(INFO) << connectionName << " MQTT: PUBLISH Resend";

            send(publish);
        }

        for (const uint16_t packetIdentifier : session->pubrelPacketIdentifierSet) {
            LOG(INFO) << connectionName << " MQTT: PUBREL Resend";

            send(iot::mqtt::packets::Pubrel(packetIdentifier));
        }

        if (keepAlive > 0) {
            keepAlive *= 1.5;

            LOG(INFO) << connectionName << " MQTT: Keep alive initialized with: " << keepAlive;

            keepAliveTimer = core::timer::Timer::singleshotTimer(
                [this, keepAlive]() {
                    LOG(ERROR) << connectionName << " MQTT: Keep-alive timer expired. Interval was: " << keepAlive;
                    mqttContext->close();
                },
                keepAlive);
        }

        mqttContext->getSocketConnection()->setTimeout(0);
    }

    void Mqtt::send(const ControlPacket& controlPacket) const {
        LOG(INFO) << connectionName << " MQTT: " << controlPacket.getName() << " send: " << clientId;

        send(controlPacket.serialize());
    }

    void Mqtt::send(const std::vector<char>& data) const {
        LOG(TRACE) << connectionName << " MQTT: Send data (full message):\n" << toHexString(data);

        mqttContext->send(data.data(), data.size());
    }

    void Mqtt::sendPublish(const std::string& topic, const std::string& message, uint8_t qoS,
                           bool retain) const { // Server & Client

        uint16_t packageIdentifier = qoS != 0 ? getPacketIdentifier() : 0;

        send(iot::mqtt::packets::Publish(packageIdentifier, topic, message, qoS, false, retain));

        LOG(INFO) << connectionName << " MQTT:   Topic: " << topic;
        LOG(INFO) << connectionName << " MQTT:   Message:\n" << toHexString(message);
        LOG(DEBUG) << connectionName << " MQTT:   QoS: " << static_cast<uint16_t>(qoS);
        LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: " << _packetIdentifier;
        LOG(DEBUG) << connectionName << " MQTT:   DUP: " << false;
        LOG(DEBUG) << connectionName << " MQTT:   Retain: " << retain;

        if (qoS == 2) {
            session->publishMap.emplace(packageIdentifier,
                                        iot::mqtt::packets::Publish(packageIdentifier, topic, message, qoS, true, retain));
        }
    }

    void Mqtt::sendPuback(uint16_t packetIdentifier) const { // Server & Client
        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void Mqtt::sendPubrec(uint16_t packetIdentifier) const { // Server & Client
        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void Mqtt::sendPubrel(uint16_t packetIdentifier) const { // Server & Client
        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void Mqtt::sendPubcomp(uint16_t packetIdentifier) const { // Server & Client
        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
    }

    void Mqtt::onPublish([[maybe_unused]] const packets::Publish& publish) {
    }

    void Mqtt::onPuback([[maybe_unused]] const iot::mqtt::packets::Puback& puback) {
    }

    void Mqtt::onPubrec([[maybe_unused]] const iot::mqtt::packets::Pubrec& pubrec) {
    }

    void Mqtt::onPubrel([[maybe_unused]] const iot::mqtt::packets::Pubrel& pubrel) {
    }

    void Mqtt::onPubcomp([[maybe_unused]] const iot::mqtt::packets::Pubcomp& pubcomp) {
    }

    bool Mqtt::_onPublish(const packets::Publish& publish) {
        bool deliver = true;

        LOG(INFO) << connectionName << " MQTT:   Topic: " << publish.getTopic();
        LOG(INFO) << connectionName << " MQTT:   Message:\n" << toHexString(publish.getMessage());
        LOG(DEBUG) << connectionName << " MQTT:   QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << connectionName << " MQTT:   DUP: " << publish.getDup();
        LOG(DEBUG) << connectionName << " MQTT:   Retain: " << publish.getRetain();

        if (publish.getQoS() > 2) {
            LOG(ERROR) << connectionName << " MQTT:   Received invalid QoS: " << publish.getQoS();
            mqttContext->end(true);
            deliver = false;
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoS() > 0) {
            LOG(ERROR) << connectionName << " MQTT:   Received QoS > 0 but no PackageIdentifier present";
            mqttContext->end(true);
            deliver = false;
        } else {
            switch (publish.getQoS()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());

                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());

                    if (session->publishPacketIdentifierSet.contains(publish.getPacketIdentifier())) {
                        deliver = false;
                    } else {
                        session->publishPacketIdentifierSet.insert(publish.getPacketIdentifier());
                    }

                    break;
            }
        }

        return deliver;
    }

    void Mqtt::_onPuback(const iot::mqtt::packets::Puback& puback) {
        if (puback.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << puback.getPacketIdentifier() << std::dec;
        }

        onPuback(puback);
    }

    void Mqtt::_onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        if (pubrec.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << pubrec.getPacketIdentifier() << std::dec;

            session->publishMap.erase(pubrec.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.insert(pubrec.getPacketIdentifier());

            sendPubrel(pubrec.getPacketIdentifier());
        }

        onPubrec(pubrec);
    }

    void Mqtt::_onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        if (pubrel.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << pubrel.getPacketIdentifier() << std::dec;

            session->publishPacketIdentifierSet.erase(pubrel.getPacketIdentifier());

            sendPubcomp(pubrel.getPacketIdentifier());
        }

        onPubrel(pubrel);
    }

    void Mqtt::_onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        if (pubcomp.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << pubcomp.getPacketIdentifier() << std::dec;

            session->publishMap.erase(pubcomp.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.erase(pubcomp.getPacketIdentifier());
        }

        onPubcomp(pubcomp);
    }

    void Mqtt::printVP(const iot::mqtt::ControlPacket& packet) const {
        LOG(INFO) << connectionName << " MQTT: " << packet.getName() << " received: " << clientId;

        const std::string hexString = toHexString(packet.serializeVP());
        if (!hexString.empty()) {
            LOG(TRACE) << connectionName << " MQTT: Received data (variable header and payload):\n" << hexString;
        }
    }

    void Mqtt::printFixedHeader(const FixedHeader& fixedHeader) const {
        LOG(INFO) << connectionName << " MQTT: ====================================";

        LOG(TRACE) << connectionName << " MQTT: Received data (fixed header):\n" << toHexString(fixedHeader.serialize());

        LOG(DEBUG) << connectionName << " MQTT: Fixed Header: PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(fixedHeader.getType()) << " (" << iot::mqtt::mqttPackageName[fixedHeader.getType()] << ")"
                   << std::dec;
        LOG(DEBUG) << connectionName << " MQTT:   PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(fixedHeader.getFlags()) << std::dec;
        LOG(DEBUG) << connectionName << " MQTT:   RemainingLength: " << fixedHeader.getRemainingLength();
    }

    std::string Mqtt::toHexString(const std::vector<char>& data) {
        const std::string hexDump = utils::hexDump(data, 32);
        return !hexDump.empty() ? std::string(32, ' ').append(hexDump) : "";
    }

    std::string Mqtt::toHexString(const std::string& data) {
        return toHexString(std::vector<char>(data.begin(), data.end()));
    }

    uint16_t Mqtt::getPacketIdentifier() const {
        ++_packetIdentifier;

        if (_packetIdentifier == 0) {
            ++_packetIdentifier;
        }

        return _packetIdentifier;
    }

} // namespace iot::mqtt
