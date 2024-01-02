/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
#include "utils/system/signal.h"

#include <cstring>
#include <iomanip>
#include <map>
#include <ostream>
#include <set>

// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    Mqtt::Mqtt(const std::string& clientId)
        : clientId(clientId) {
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
                    LOG(DEBUG) << "MQTT: Received packet-type is unavailable ... closing connection";

                    mqttContext->end(true);
                    break;
                }
                if (controlPacketDeserializer->isError()) {
                    LOG(DEBUG) << "MQTT: Fixed header has error ... closing connection";

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
                    LOG(DEBUG) << "MQTT: Control packet has error ... closing connection";
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
        LOG(INFO) << "MQTT: Disconnected";
    }

    void Mqtt::onExit(int sig) {
        LOG(INFO) << "MQTT: Exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig << ")";
    }

    core::socket::stream::SocketConnection* Mqtt::getSocketConnection() {
        return mqttContext->getSocketConnection();
    }

    void Mqtt::initSession(Session* session, utils::Timeval keepAlive) {
        this->session = session;

        for (const auto& [packetIdentifier, publish] : session->publishMap) {
            LOG(DEBUG) << "MQTT: PUBLISH Resend";

            send(publish);
        }

        for (const uint16_t packetIdentifier : session->pubrelPacketIdentifierSet) {
            LOG(DEBUG) << "MQTT: PUBREL Resend";

            send(iot::mqtt::packets::Pubrel(packetIdentifier));
        }

        if (keepAlive > 0) {
            keepAlive *= 1.5;

            LOG(TRACE) << "MQTT: Keep alive initialized with: " << keepAlive;

            keepAliveTimer = core::timer::Timer::singleshotTimer(
                [this, keepAlive]() -> void {
                    LOG(DEBUG) << "MQTT: Keep-alive timer expired. Interval was: " << keepAlive;
                    mqttContext->close();
                },
                keepAlive);

            getSocketConnection()->setTimeout(0);
        }
    }

    void Mqtt::send(const ControlPacket& controlPacket) const {
        LOG(INFO) << "MQTT: " << controlPacket.getName() << " send: " << clientId;

        send(controlPacket.serialize());
    }

    void Mqtt::send(const std::vector<char>& data) const {
        LOG(TRACE) << "MQTT: Send data:\n" << dataToHexString(data);

        mqttContext->send(data.data(), data.size());
    }

    void Mqtt::sendPublish(const std::string& topic, const std::string& message, uint8_t qoS,
                           bool retain) { // Server & Client

        uint16_t packageIdentifier = qoS != 0 ? getPacketIdentifier() : 0;

        send(iot::mqtt::packets::Publish(packageIdentifier, topic, message, qoS, false, retain));

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

        LOG(DEBUG) << "MQTT:   Topic: " << publish.getTopic();
        LOG(DEBUG) << "MQTT:   Message:\n" << stringToHexString(publish.getMessage());
        LOG(DEBUG) << "MQTT:   QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << "MQTT:   PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "MQTT:   DUP: " << publish.getDup();
        LOG(DEBUG) << "MQTT:   Retain: " << publish.getRetain();

        if (publish.getQoS() > 2) {
            LOG(DEBUG) << "MQTT:   Received invalid QoS: " << publish.getQoS();
            mqttContext->end(true);
            deliver = false;
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoS() > 0) {
            LOG(DEBUG) << "MQTT:   Received QoS > 0 but no PackageIdentifier present";
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
            LOG(DEBUG) << "MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << puback.getPacketIdentifier();
        }

        onPuback(puback);
    }

    void Mqtt::_onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        if (pubrec.getPacketIdentifier() == 0) {
            LOG(DEBUG) << "MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrec.getPacketIdentifier();

            session->publishMap.erase(pubrec.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.insert(pubrec.getPacketIdentifier());

            sendPubrel(pubrec.getPacketIdentifier());
        }

        onPubrec(pubrec);
    }

    void Mqtt::_onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        if (pubrel.getPacketIdentifier() == 0) {
            LOG(DEBUG) << "MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrel.getPacketIdentifier();

            session->publishPacketIdentifierSet.erase(pubrel.getPacketIdentifier());

            sendPubcomp(pubrel.getPacketIdentifier());
        }

        onPubrel(pubrel);
    }

    void Mqtt::_onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        if (pubcomp.getPacketIdentifier() == 0) {
            LOG(DEBUG) << "MQTT:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT:   PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubcomp.getPacketIdentifier();

            session->publishMap.erase(pubcomp.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.erase(pubcomp.getPacketIdentifier());
        }

        onPubcomp(pubcomp);
    }

    void Mqtt::printVP(const iot::mqtt::ControlPacket& packet) const {
        LOG(TRACE) << "MQTT: Received data (variable header and payload):\n" << dataToHexString(packet.serializeVP());

        LOG(INFO) << "MQTT: " << packet.getName() << " received: " << clientId;
    }

    void Mqtt::printFixedHeader(const FixedHeader& fixedHeader) {
        LOG(DEBUG) << "MQTT: ======================================================";

        LOG(TRACE) << "MQTT: Received data (fixed header):\n" << dataToHexString(fixedHeader.serialize());

        LOG(DEBUG) << "MQTT: Fixed Header: PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(fixedHeader.getType()) << " (" << iot::mqtt::mqttPackageName[fixedHeader.getType()] << ")";
        LOG(DEBUG) << "MQTT:   PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<uint16_t>(fixedHeader.getFlags()) << std::dec;
        LOG(DEBUG) << "MQTT:   RemainingLength: " << fixedHeader.getRemainingLength();
    }

    std::string Mqtt::dataToHexString(const std::vector<char>& data) {
        std::stringstream ss;

        ss << "                                               ";

        if (!data.empty()) {
            unsigned long i = 0;
            for (const char ch : data) {
                if (i != 0 && i % 8 == 0 && i != data.size()) {
                    ss << std::endl;
                    ss << "                                               ";
                }
                ++i;
                ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
                   << " "; // << " | ";
            }
        } else {
            ss << "<EMPTY>";
        }

        return ss.str();
    }

    std::string Mqtt::stringToHexString(const std::string& data) {
        return dataToHexString(std::vector<char>(data.begin(), data.end()));
    }

    uint16_t Mqtt::getPacketIdentifier() {
        ++_packetIdentifier;

        if (_packetIdentifier == 0) {
            ++_packetIdentifier;
        }

        return _packetIdentifier;
    }

} // namespace iot::mqtt
