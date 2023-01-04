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
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class SubscribtionTree {
    public:
        explicit SubscribtionTree(iot::mqtt::server::broker::Broker* broker);

        void publishRetained(const std::string& clientId);

        bool subscribe(const std::string& topic, const std::string& clientId, uint8_t clientQoS);

        void publish(Message&& message);

        bool unsubscribe(std::string topic, const std::string& clientId);
        bool unsubscribe(const std::string& clientId);

    private:
        class SubscribtionTreeNode {
        public:
            explicit SubscribtionTreeNode(iot::mqtt::server::broker::Broker* broker);

            void publishRetained(const std::string& clientId);

            bool subscribe(const std::string& topic,
                           const std::string& clientId,
                           uint8_t clientQoS,
                           std::string remainingTopicName,
                           bool leafFound);

            void publish(Message& message, std::string remainingTopicName, bool leafFound);

            bool unsubscribe(const std::string& clientId, std::string remainingTopicName, bool leafFound);
            bool unsubscribe(const std::string& clientId);

        private:
            std::map<std::string, uint8_t> subscribers;
            std::map<std::string, SubscribtionTreeNode> subscribtions;

            std::string subscribedTopicName = "";

            iot::mqtt::server::broker::Broker* broker;
        };

        SubscribtionTreeNode head;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H
