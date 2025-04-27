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
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client {

    Mqtt::Mqtt(const std::string& connectionName, const std::string& clientId, const std::string& sessionStoreFileName)
        : Super(connectionName, clientId)
        , sessionStoreFileName(sessionStoreFileName) { // NOLINT
        if (!sessionStoreFileName.empty()) {
            std::ifstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                try {
                    nlohmann::json sessionStoreJson;

                    sessionStoreFile >> sessionStoreJson;

                    session.fromJson(sessionStoreJson);

                    LOG(DEBUG) << connectionName << " MQTT Client:   ... Persistent session data loaded successful";
                } catch (const nlohmann::json::exception&) {
                    LOG(DEBUG) << connectionName << " MQTT Client:   ... Starting with empty session: Session store '"
                               << sessionStoreFileName << "' empty or corrupted";

                    session.clear();
                }

                sessionStoreFile.close();
                std::remove(sessionStoreFileName.data()); // NOLINT

                LOG(INFO) << connectionName << " MQTT Client: Restoring saved session done";
            } else {
                PLOG(WARNING) << connectionName << " MQTT Client:   ... Could not read session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << connectionName << " MQTT Client: Session not reloaded: Session store filename empty";
        }
    }

    Mqtt::~Mqtt() {
        if (!sessionStoreFileName.empty()) {
            const nlohmann::json sessionJson = session.toJson();

            std::ofstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                if (!sessionJson.empty()) {
                    sessionStoreFile << sessionJson;
                }

                sessionStoreFile.close();
            } else {
                PLOG(DEBUG) << connectionName << " MQTT Client: Could not write session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << connectionName << " MQTT Client: Session not saved: Session store filename empty";
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
        static_cast<iot::mqtt::client::ControlPacketDeserializer*>(controlPacketDeserializer)->deliverPacket(this); // NOLINT
    }

    bool Mqtt::onSignal([[maybe_unused]] int sig) {
        return false;
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
        LOG(INFO) << connectionName << " MQTT Client:   Acknowledge Flag: " << static_cast<int>(connack.getAcknowledgeFlags());
        LOG(INFO) << connectionName << " MQTT Client:   Return code: " << static_cast<int>(connack.getReturnCode());
        LOG(INFO) << connectionName << " MQTT Client:   Session present: " << connack.getSessionPresent();

        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            LOG(ERROR) << connectionName << " MQTT Client:   Negative ack received";
        } else {
            initSession(&session, keepAlive);

            pingTimer = core::timer::Timer::intervalTimer(
                [this]() {
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
            LOG(ERROR) << connectionName << " MQTT Client:   PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT Client:  PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << suback.getPacketIdentifier() << std::dec;

            std::stringstream ss;
            std::list<uint8_t>::size_type i = 0;

            for (const uint8_t returnCode : suback.getReturnCodes()) {
                if (i != 0 && i % 8 == 0 && i != suback.getReturnCodes().size()) {
                    ss << std::endl;
                    ss << "                                                       ";
                }
                ++i;
                ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(returnCode) << " "; // << " | ";
            }

            LOG(DEBUG) << connectionName << " MQTT Client:  Return codes: " << ss.str();

            onSuback(suback);
        }
    }

    void Mqtt::_onUnsuback(const iot::mqtt::client::packets::Unsuback& unsuback) {
        if (unsuback.getPacketIdentifier() == 0) {
            LOG(ERROR) << connectionName << " MQTT Client:  PacketIdentifier missing";
            mqttContext->end(true);
        } else {
            LOG(DEBUG) << connectionName << " MQTT Client:  PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4)
                       << unsuback.getPacketIdentifier() << std::dec;

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
                           bool loopPrevention) { // Client
        this->clientId = clientId;

        LOG(INFO) << connectionName << " MQTT Client: CONNECT send: " << clientId;

        send(iot::mqtt::packets::Connect(
            clientId, keepAlive, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password, loopPrevention));

        this->keepAlive = keepAlive;
    }

    void Mqtt::sendSubscribe(const std::list<iot::mqtt::Topic>& topics) { // Client
        send(iot::mqtt::packets::Subscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendUnsubscribe(const std::list<std::string>& topics) { // Client
        send(iot::mqtt::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void Mqtt::sendPingreq() const { // Client
        send(iot::mqtt::packets::Pingreq());
    }

    void Mqtt::sendDisconnect() const { // Client
        send(iot::mqtt::packets::Disconnect());

        mqttContext->end();
    }

} // namespace iot::mqtt::client
