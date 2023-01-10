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

#ifndef IOT_MQTT_SERVER_BROKER_MESSAGE_H
#define IOT_MQTT_SERVER_BROKER_MESSAGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class Message {
    public:
        Message() = default;
        Message(const std::string& topic, const std::string& message, uint8_t qoS);
        Message(const Message& message) = default;

        Message& operator=(const Message&) = default;

        const std::string& getTopic() const;
        void setTopic(const std::string& newTopic);

        const std::string& getMessage() const;
        void setMessage(const std::string& newMessage);

        uint8_t getQoS() const;
        void setQoS(uint8_t newQoS);

        nlohmann::json toJson() const;
        Message& fromJson(const nlohmann::json& messageJson);

    private:
        std::string topic;
        std::string message;
        uint8_t qoS = 0;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_MESSAGE_H
