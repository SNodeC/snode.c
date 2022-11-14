/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_DUP_FALSE false
#define MQTT_DUP_TRUE true
#define MQTT_RETAIN_FALSE false
#define MQTT_RETAIN_TRUE true

namespace iot::mqtt::server::broker {

    class Message {
    public:
        Message() = default;
        Message(const std::string& topic, const std::string& message, uint8_t qoS, bool retain, bool dup);
        Message(const Message&) = default;

        Message& operator=(const Message&) = default;

        const std::string& getTopic() const;
        const std::string& getMessage() const;
        uint8_t getQoS() const;

        void setQoS(uint8_t qoS);

        bool getRetain() const;
        bool getDup() const;

    private:
        std::string topic;
        std::string message;
        uint8_t qoS = 0;
        bool retain = false;
        bool dup = false;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_MESSAGE_H
