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

#include "iot/mqtt/server/broker/Session.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <nlohmann/json.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Session::Session(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt)
        : clientId(clientId)
        , mqtt(mqtt) {
    }

    Session::Session(const std::string& clientId, const nlohmann::json& sessionJson)
        : iot::mqtt::Session(sessionJson)
        , clientId(clientId) {
        std::vector<nlohmann::json> messageVector = sessionJson["message_queue"];

        for (const nlohmann::json& messageJson : messageVector) {
            messageQueue.push_back(Message(messageJson["topic"], messageJson["message"], messageJson["qos"], messageJson["retain"]));
            LOG(TRACE) << "Message: " << messageJson["topic"] << ":" << messageJson["message"];
        }
    }

    void Session::sendPublish(Message& message, uint8_t qoS) {
        LOG(TRACE) << "  Send Publish: ClientId: " << clientId;

        std::stringstream messageData;
        const std::string& mes = message.getMessage();

        unsigned long i = 0;
        for (char ch : mes) {
            if (i != 0 && i % 8 == 0 && i != mes.size()) {
                messageData << std::endl;
                messageData << "                                                                  ";
            }
            ++i;
            messageData << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
                        << " "; // << " | ";
        }

        LOG(TRACE) << "                TopicName: " << message.getTopic();
        LOG(TRACE) << "                Message: " << messageData.str();
        LOG(TRACE) << "                QoS: " << static_cast<uint16_t>(std::min(qoS, message.getQoS()));

        if (isActive()) {
            mqtt->sendPublish(message.getTopic(), message.getMessage(), std::min(message.getQoS(), qoS), message.getRetain());
        } else {
            if (message.getQoS() == 0) {
                messageQueue.clear();
            }

            messageQueue.push_back(Message(message, std::min(message.getQoS(), qoS)));
        }
    }

    void Session::publishQueued() {
        LOG(TRACE) << "    send queued messages ...";
        for (iot::mqtt::server::broker::Message& message : messageQueue) {
            sendPublish(message, message.getQoS());
        }
        LOG(TRACE) << "    ... done";

        messageQueue.clear();
    }

    void Session::renew(iot::mqtt::server::Mqtt* mqtt) {
        this->mqtt = mqtt;
    }

    void Session::retain() {
        this->mqtt = nullptr;
    }

    bool Session::isActive() const {
        return mqtt != nullptr;
    }

    bool Session::isOwnedBy(const iot::mqtt::server::Mqtt* mqtt) const {
        return this->mqtt == mqtt;
    }

    nlohmann::json Session::toJson() {
        nlohmann::json json = iot::mqtt::Session::toJson();

        std::vector<nlohmann::json> messageVector;
        for (const Message& message : messageQueue) {
            nlohmann::json messageJson;

            messageJson["topic"] = message.getTopic();
            messageJson["message"] = message.getMessage();
            messageJson["qos"] = message.getQoS();
            messageJson["retain"] = message.getRetain();

            messageVector.emplace_back(messageJson);
        }

        json["message_queue"] = messageVector;

        return json;
    }

} // namespace iot::mqtt::server::broker
