/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/client/Mqtt.h"

#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/client/packets/Connack.h"
#include "iot/mqtt/client/packets/Pingresp.h"
#include "iot/mqtt/client/packets/Puback.h"
#include "iot/mqtt/client/packets/Pubcomp.h"
#include "iot/mqtt/client/packets/Publish.h"
#include "iot/mqtt/client/packets/Pubrec.h"
#include "iot/mqtt/client/packets/Pubrel.h"
#include "iot/mqtt/client/packets/Suback.h"
#include "iot/mqtt/client/packets/Unsuback.h"
#include "iot/mqtt/packets/Connect.h"
#include "iot/mqtt/packets/Disconnect.h"
#include "iot/mqtt/packets/Pingreq.h"
#include "iot/mqtt/packets/Subscribe.h"
#include "iot/mqtt/packets/Unsubscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    Mqtt::Mqtt()
        : sessionStoreFileName((getenv("MQTT_SESSION_STORE") != nullptr) ? getenv("MQTT_SESSION_STORE") : "") { // NOLINT
        std::ifstream sessionStoreFile(sessionStoreFileName);

        if (sessionStoreFile.is_open()) {
            try {
                nlohmann::json sessionStoreJson;

                sessionStoreFile >> sessionStoreJson;
                LOG(INFO) << sessionStoreJson.dump(4);

                session.fromJson(sessionStoreJson);

                LOG(INFO) << "Persistent session data loaded successfull";
            } catch (const nlohmann::json::exception& e) {
                LOG(INFO) << "Starting with empty session: session store empty or corrupted";
                LOG(TRACE) << sessionStoreFileName << ": " << e.what();

                session.clear();
            }
            sessionStoreFile.close();
            std::remove(sessionStoreFileName.data());
        }
    }

    Mqtt::~Mqtt() {
        if (!sessionStoreFileName.empty()) {
            nlohmann::json sessionJson = session.toJson();

            std::ofstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                if (!sessionJson.empty()) {
                    sessionStoreFile << sessionJson;
                }

                sessionStoreFile.close();
            }
        } else {
            LOG(INFO) << "Session not saved: Sessionstore filename empty";
        }

        pingTimer.cancel();
    }

    iot::mqtt::ControlPacketDeserializer* Mqtt::createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) {
        iot::mqtt::ControlPacketDeserializer* currentPacket = nullptr;

        switch (fixedHeader.getPacketType()) {
            case MQTT_CONNACK:
                currentPacket = new iot::mqtt::client::packets::Connack(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBLISH:
                currentPacket = new iot::mqtt::client::packets::Publish(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBACK:
                currentPacket = new iot::mqtt::client::packets::Puback(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREC:
                currentPacket = new iot::mqtt::client::packets::Pubrec(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBREL:
                currentPacket = new iot::mqtt::client::packets::Pubrel(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PUBCOMP:
                currentPacket = new iot::mqtt::client::packets::Pubcomp(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_SUBACK:
                currentPacket = new iot::mqtt::client::packets::Suback(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_UNSUBACK:
                currentPacket = new iot::mqtt::client::packets::Unsuback(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            case MQTT_PINGRESP:
                currentPacket = new iot::mqtt::client::packets::Pingresp(fixedHeader.getRemainingLength(), fixedHeader.getFlags());
                break;
            default:
                currentPacket = nullptr;
                break;
        }

        return currentPacket;
    }

    void Mqtt::deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) {
        static_cast<iot::mqtt::client::ControlPacketDeserializer*>(controlPacketDeserializer)->deliverPacket(this);
    }

    void Mqtt::onConnack([[maybe_unused]] const mqtt::packets::Connack& connack) {
    }

    void Mqtt::onSuback([[maybe_unused]] const mqtt::packets::Suback& suback) {
    }

    void Mqtt::onUnsuback([[maybe_unused]] const mqtt::packets::Unsuback& unsuback) {
    }

    void Mqtt::onPingresp([[maybe_unused]] const mqtt::packets::Pingresp& pingresp) {
    }

    void Mqtt::_onConnack(const iot::mqtt::client::packets::Connack& connack) {
        LOG(DEBUG) << "Received Connack:";
        LOG(DEBUG) << "=================";
        printStandardHeader(connack);
        LOG(DEBUG) << "Acknowledge Flag: " << static_cast<int>(connack.getAcknowledgeFlags());
        LOG(DEBUG) << "Return code: " << static_cast<int>(connack.getReturnCode());
        LOG(DEBUG) << "Session present: " << connack.getSessionPresent();

        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            LOG(TRACE) << "Negative ack received";
            mqttContext->end(true);
        } else {
            initSession(&session, keepAlive * 2);

            pingTimer = core::timer::Timer::intervalTimer(
                [this](void) -> void {
                    sendPingreq();
                },
                keepAlive);

            onConnack(connack);
        }
    }

    void Mqtt::_onPublish(const iot::mqtt::client::packets::Publish& publish) {
        LOG(DEBUG) << "Received PUBLISH:";

        if (Super::_onPublish(publish)) {
            onPublish(publish);
        }
    }

    void Mqtt::_onSuback(const iot::mqtt::client::packets::Suback& suback) {
        LOG(DEBUG) << "Received SUBACK:";
        LOG(DEBUG) << "================";
        printStandardHeader(suback);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << suback.getPacketIdentifier();

        std::stringstream ss;
        std::list<uint8_t>::size_type i = 0;

        for (uint8_t returnCode : suback.getReturnCodes()) {
            if (i != 0 && i % 8 == 0 && i != suback.getReturnCodes().size()) {
                ss << std::endl;
                ss << "                                                       ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(returnCode) << " "; // << " | ";
        }

        LOG(DEBUG) << "Return codes: " << ss.str();

        if (suback.getPacketIdentifier() == 0) {
            LOG(TRACE) << "PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            onSuback(suback);
        }
    }

    void Mqtt::_onUnsuback(const iot::mqtt::client::packets::Unsuback& unsuback) {
        LOG(DEBUG) << "Received UNSUBACK:";
        LOG(DEBUG) << "==================";
        printStandardHeader(unsuback);
        LOG(DEBUG) << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << unsuback.getPacketIdentifier();

        if (unsuback.getPacketIdentifier() == 0) {
            LOG(TRACE) << "PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            onUnsuback(unsuback);
        }
    }

    void Mqtt::_onPingresp(const iot::mqtt::client::packets::Pingresp& pingresp) {
        LOG(DEBUG) << "Received PINGRESP:";
        LOG(DEBUG) << "==================";
        printStandardHeader(pingresp);

        onPingresp(pingresp);
    }

    void Mqtt::sendConnect(uint16_t keepAlive,
                           const std::string& clientId,
                           bool cleanSession,
                           const std::string& willTopic,
                           const std::string& willMessage,
                           uint8_t willQoS,
                           bool willRetain,
                           const std::string& username,
                           const std::string& password) { // Client
        LOG(DEBUG) << "Send CONNECT";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Connect(
            clientId, keepAlive, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password));

        this->keepAlive = keepAlive;
    }

    void Mqtt::sendSubscribe(std::list<iot::mqtt::Topic>& topics) { // Client
        LOG(DEBUG) << "Send SUBSCRIBE";
        LOG(DEBUG) << "==============";

        send(iot::mqtt::packets::Subscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendUnsubscribe(std::list<std::string>& topics) { // Client
        LOG(DEBUG) << "Send UNSUBSCRIBE";
        LOG(DEBUG) << "================";

        send(iot::mqtt::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendPingreq() const { // Client
        LOG(DEBUG) << "Send Pingreq";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Pingreq());
    }

    void Mqtt::sendDisconnect() const {
        LOG(DEBUG) << "Send Disconnect";
        LOG(DEBUG) << "===============";

        send(iot::mqtt::packets::Disconnect());
    }

} // namespace iot::mqtt::client
