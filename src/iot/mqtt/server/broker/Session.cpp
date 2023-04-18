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
#include <nlohmann/json.hpp>
#include <string>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Session::Session(iot::mqtt::server::Mqtt* mqtt)
        : mqtt(mqtt) {
    }

    void Session::sendPublish(Message& message, uint8_t qoS, bool retain) {
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
            mqtt->sendPublish(message.getTopic(), message.getMessage(), std::min(message.getQoS(), qoS), retain);
        } else {
            if (message.getQoS() == 0) {
                messageQueue.clear();
            }

            message.setQoS(std::min(message.getQoS(), qoS));
            messageQueue.emplace_back(message);
        }
    }

    void Session::publishQueued() {
        LOG(TRACE) << "    send queued messages ...";
        for (iot::mqtt::server::broker::Message& message : messageQueue) {
            sendPublish(message, message.getQoS(), false);
        }
        LOG(TRACE) << "    ... done";

        messageQueue.clear();
    }

    Session* Session::renew(iot::mqtt::server::Mqtt* mqtt) {
        this->mqtt = mqtt;

        return this;
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
            messageVector.emplace_back(message.toJson());
        }

        json["message_queue"] = messageVector;

        return json;
    }

    void Session::fromJson(const nlohmann::json& json) {
        for (const nlohmann::json& messageJson : json["message_queue"]) {
            messageQueue.emplace_back(Message().fromJson(messageJson));
        }

        iot::mqtt::Session::fromJson(json);
    }

} // namespace iot::mqtt::server::broker
