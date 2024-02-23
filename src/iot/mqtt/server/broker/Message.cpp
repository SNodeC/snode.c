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

#include "iot/mqtt/server/broker/Message.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Message::Message(
        const std::string& originClientId, const std::string& topic, const std::string& message, uint8_t qoS, bool originRetain)
        : originClientId(originClientId)
        , topic(topic)
        , message(message)
        , qoS(qoS)
        , originRetain(originRetain) {
    }

    const std::string& Message::getOriginClientId() const {
        return originClientId;
    }

    const std::string& Message::getTopic() const {
        return topic;
    }

    void Message::setTopic(const std::string& topic) {
        this->topic = topic;
    }

    const std::string& Message::getMessage() const {
        return message;
    }

    void Message::setMessage(const std::string& message) {
        this->message = message;
    }

    uint8_t Message::getQoS() const {
        return qoS;
    }

    void Message::setQoS(uint8_t qoS) {
        this->qoS = qoS;
    }

    bool Message::getOriginRetain() const {
        return originRetain;
    }

    nlohmann::json Message::toJson() const {
        nlohmann::json json;

        json["origin_client_id"] = originClientId;
        json["topic"] = topic;
        json["message"] = message;
        json["qos"] = qoS;

        return json;
    }

    Message& Message::fromJson(const nlohmann::json& json) {
        if (!json.empty()) {
            originClientId = json["origin_client_id"];
            topic = json["topic"];
            message = json["message"];
            qoS = json["qos"];
        }

        return *this;
    }

} // namespace iot::mqtt::server::broker
