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

#ifndef IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H
#define IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H

namespace iot::mqtt::server::broker {
    class Broker;
    class Message;
} // namespace iot::mqtt::server::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class SubscribtionTree {
    public:
        explicit SubscribtionTree(iot::mqtt::server::broker::Broker* broker);

        void appear(const std::string& clientId);

        bool subscribe(const std::string& topic, const std::string& clientId, uint8_t qoS);

        void publish(Message&& message);

        bool unsubscribe(std::string topic, const std::string& clientId);
        bool unsubscribe(const std::string& clientId);

        nlohmann::json toJson();
        void fromJson(const nlohmann::json& json);

        void clear() {
            head.clear();
        }

    private:
        class TopicLevel {
        public:
            explicit TopicLevel(iot::mqtt::server::broker::Broker* broker);

            void appear(const std::string& clientId, const std::string& topic);

            bool subscribe(const std::string& clientId, uint8_t qoS, std::string topic, bool leafFound);

            void publish(Message& message, std::string topic, bool leafFound);

            bool unsubscribe(const std::string& clientId, std::string topic, bool leafFound);
            bool unsubscribe(const std::string& clientId);

            TopicLevel& fromJson(const nlohmann::json& json);
            nlohmann::json toJson();

            void clear();

        private:
            std::map<std::string, uint8_t> subscribers;

            std::map<std::string, TopicLevel> topicLevels;

            iot::mqtt::server::broker::Broker* broker;
        };

        TopicLevel head;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H
