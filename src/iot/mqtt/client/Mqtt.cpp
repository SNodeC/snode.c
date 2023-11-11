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

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client {

    Mqtt::Mqtt(const std::string& clientId)
        : Super(clientId)
        , sessionStoreFileName((getenv("MQTT_SESSION_STORE") != nullptr) ? getenv("MQTT_SESSION_STORE") : "") { // NOLINT
        if (!sessionStoreFileName.empty()) {
            std::ifstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                try {
                    nlohmann::json sessionStoreJson;

                    sessionStoreFile >> sessionStoreJson;

                    session.fromJson(sessionStoreJson);

                    LOG(DEBUG) << "MQTT Client:   ... Persistent session data loaded successfull";
                } catch (const nlohmann::json::exception&) {
                    LOG(DEBUG) << "MQTT Client:   ... Starting with empty session: Session store '" << sessionStoreFileName
                               << "' empty or corrupted";

                    session.clear();
                }

                sessionStoreFile.close();
                std::remove(sessionStoreFileName.data());

                LOG(INFO) << "MQTT Client: Restoring safed session done";
            } else {
                PLOG(INFO) << "MQTT Client:   ... Could not read session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << "MQTT Client: Session not reloaded: Session store filename empty";
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
            } else {
                PLOG(DEBUG) << "MQTT Client: Could not write session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << "MQTT Client: Session not saved: Session store filename empty";
        }

        pingTimer.cancel();
    }

    iot::mqtt::ControlPacketDeserializer* Mqtt::createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) {
        iot::mqtt::ControlPacketDeserializer* currentPacket = nullptr;

        switch (fixedHeader.getType()) {
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
        LOG(DEBUG) << "MQTT Client:   Acknowledge Flag: " << static_cast<int>(connack.getAcknowledgeFlags());
        LOG(DEBUG) << "MQTT Client:   Return code: " << static_cast<int>(connack.getReturnCode());
        LOG(DEBUG) << "MQTT Client:   Session present: " << connack.getSessionPresent();

        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            LOG(DEBUG) << "MQTT Client:   Negative ack received";
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
        if (Super::_onPublish(publish)) {
            onPublish(publish);
        }
    }

    void Mqtt::_onSuback(const iot::mqtt::client::packets::Suback& suback) {
        if (suback.getPacketIdentifier() == 0) {
            LOG(DEBUG) << "MQTT Client:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT Client:  PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << suback.getPacketIdentifier();

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

            LOG(DEBUG) << "MQTT Client:  Return codes: " << ss.str();

            onSuback(suback);
        }
    }

    void Mqtt::_onUnsuback(const iot::mqtt::client::packets::Unsuback& unsuback) {
        if (unsuback.getPacketIdentifier() == 0) {
            LOG(DEBUG) << "MQTT Client:PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << "MQTT Client:PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << unsuback.getPacketIdentifier();

            onUnsuback(unsuback);
        }
    }

    void Mqtt::_onPingresp(const iot::mqtt::client::packets::Pingresp& pingresp) {
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
                           const std::string& password,
                           bool reflect) { // Client
        this->clientId = clientId.empty() ? "<unnamed>" : clientId;

        LOG(DEBUG) << "MQTT Client: CONNECT send: " << clientId;

        send(iot::mqtt::packets::Connect(
            clientId, keepAlive, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password, reflect));

        this->keepAlive = keepAlive;
    }

    void Mqtt::sendSubscribe(std::list<iot::mqtt::Topic>& topics) { // Client
        send(iot::mqtt::packets::Subscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendUnsubscribe(std::list<std::string>& topics) { // Client
        send(iot::mqtt::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendPingreq() const { // Client
        send(iot::mqtt::packets::Pingreq());
    }

    void Mqtt::sendDisconnect() const { // Client
        send(iot::mqtt::packets::Disconnect());
    }

} // namespace iot::mqtt::client
