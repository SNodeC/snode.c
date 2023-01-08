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

#include "iot/mqtt/server/broker/Message.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json.hpp>

// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Message::Message(const std::string& topic, const std::string& message, uint8_t qoS, bool retain)
        : topic(topic)
        , message(message)
        , qoS(qoS)
        , retain(retain) {
    }

    Message::Message(const nlohmann::json& messageJson)
        : Message(messageJson["topic"], messageJson["message"], messageJson["qos"], messageJson["retain"]) {
    }

    Message::Message(const Message& message, uint8_t qoS)
        : Message(message) {
        this->qoS = qoS;
    }

    const std::string& Message::getTopic() const {
        return topic;
    }

    const std::string& Message::getMessage() const {
        return message;
    }

    uint8_t Message::getQoS() const {
        return qoS;
    }

    bool Message::getRetain() const {
        return retain;
    }

    nlohmann::json Message::toJson() const {
        nlohmann::json messageJson;

        messageJson["topic"] = topic;
        messageJson["message"] = message;
        messageJson["qos"] = qoS;
        messageJson["retain"] = retain;

        return messageJson;
    }

} // namespace iot::mqtt::server::broker
