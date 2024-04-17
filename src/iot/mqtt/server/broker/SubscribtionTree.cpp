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

#include "iot/mqtt/server/broker/SubscribtionTree.h"

#include "iot/mqtt/Mqtt.h"
#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    SubscribtionTree::SubscribtionTree(iot::mqtt::server::broker::Broker* broker)
        : head(broker, "") {
    }

    void SubscribtionTree::appear(const std::string& clientId) {
        head.appear(clientId, "");
    }

    bool SubscribtionTree::subscribe(const std::string& topic, const std::string& clientId, uint8_t qoS) {
        bool success = false;

        auto hashCount = std::ranges::count(topic, '#');
        if (!topic.empty() && (hashCount == 0 || (hashCount == 1 && topic.ends_with('#')))) {
            head.subscribe(clientId, qoS, topic);

            success = true;
        } else {
            LOG(DEBUG) << "MQTT Broker: Subscribe: Wrong '#' placement: " << topic;
        }

        return success;
    }

    void SubscribtionTree::publish(Message&& message) {
        auto hashCount = std::ranges::count(message.getTopic(), '#');
        if (!message.getTopic().empty() && (hashCount == 0 || (hashCount == 1 && message.getTopic().ends_with('#')))) {
            head.publish(message, message.getTopic());
        } else {
            LOG(DEBUG) << "MQTT Broker: Publish: Wrong '#' placement: " << message.getTopic();
        }
    }

    void SubscribtionTree::unsubscribe(const std::string& topic, const std::string& clientId) {
        auto hashCount = std::ranges::count(topic, '#');
        if (!topic.empty() && (hashCount == 0 || (hashCount == 1 && topic.ends_with('#')))) {
            head.unsubscribe(clientId, topic);
        } else {
            LOG(DEBUG) << "MQTT Broker: Unsubscribe: Wrong '#' placement: " << topic;
        }
    }

    void SubscribtionTree::unsubscribe(const std::string& clientId) {
        head.unsubscribe(clientId);
    }

    SubscribtionTree::TopicLevel::TopicLevel(Broker* broker, const std::string& topicLevel)
        : broker(broker)
        , topicLevel(topicLevel) {
    }

    void SubscribtionTree::TopicLevel::appear(const std::string& clientId, const std::string& topic) {
        if (clientIds.contains(clientId)) {
            broker->appear(clientId, topic, clientIds[clientId]);
        }

        for (auto& [topicLevel, subscribtion] : topicLevels) { // cppcheck-suppress shadowVariable
            subscribtion.appear(clientId, std::string(topic).append(topic.empty() ? "" : "/").append(topicLevel));
        }
    }

    bool SubscribtionTree::TopicLevel::subscribe(const std::string& clientId, uint8_t qoS, std::string topic) {
        if (topic.empty()) {
            LOG(DEBUG) << "MQTT Broker: Subscribe";
            LOG(DEBUG) << "MQTT Broker:   ClientId: " << clientId;

            clientIds[clientId] = qoS;
        } else {
            const std::string topicLevel = topic.substr(0, topic.find('/')); // cppcheck-suppress shadowVariable

            topic.erase(0, topicLevel.size() + 1);

            const auto& [it, inserted] = topicLevels.insert({topicLevel, SubscribtionTree::TopicLevel(broker, topicLevel)});

            if (!it->second.subscribe(clientId, qoS, topic)) {
                LOG(DEBUG) << "MQTT Broker:   Erase topic: " << topicLevel << " /" << topic;

                topicLevels.erase(it);
            } else {
                LOG(DEBUG) << "MQTT Broker:   Topic: " << topicLevel << " /" << topic;
            }
        }

        return !topicLevels.empty() || !clientIds.empty();
    }

    void SubscribtionTree::TopicLevel::publish(Message& message, std::string topic) {
        if (topic.empty()) {
            LOG(DEBUG) << "MQTT Broker: Found match:";
            LOG(DEBUG) << "MQTT Broker:   Topic: '" << message.getTopic() << "';";
            LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
            LOG(DEBUG) << "MQTT Broker:   Distribute Publish for match ...";

            for (auto& [clientId, clientQoS] : clientIds) {
                broker->sendPublish(clientId, message, clientQoS, false);
            }

            LOG(DEBUG) << "MQTT Broker:   ... completed!";

            const auto nextHashLevel = topicLevels.find("#");
            if (nextHashLevel != topicLevels.end()) {
                LOG(DEBUG) << "MQTT Broker: Found parent match:";
                LOG(DEBUG) << "MQTT Broker:   Topic: '" << message.getTopic() << "'";
                LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
                LOG(DEBUG) << "MQTT Broker:   Distribute Publish for match ...";

                for (auto& [clientId, clientQoS] : nextHashLevel->second.clientIds) {
                    broker->sendPublish(clientId, message, clientQoS, false);
                }

                LOG(DEBUG) << "MQTT Broker:   ... completed!";
            }
        } else {
            const std::string topicLevel = topic.substr(0, topic.find('/')); // cppcheck-suppress shadowVariable

            topic.erase(0, topicLevel.size() + 1);

            auto foundNode = topicLevels.find(topicLevel);
            if (foundNode != topicLevels.end()) {
                foundNode->second.publish(message, topic);
            }

            foundNode = topicLevels.find("+");
            if (foundNode != topicLevels.end()) {
                foundNode->second.publish(message, topic);
            }

            foundNode = topicLevels.find("#");
            if (foundNode != topicLevels.end()) {
                LOG(DEBUG) << "MQTT Broker: Found match:";
                LOG(DEBUG) << "MQTT Broker:   Topic: '" << message.getTopic() << "';";
                LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
                LOG(DEBUG) << "MQTT Broker:   Distribute Publish for match ...";

                for (auto& [clientId, clientQoS] : foundNode->second.clientIds) {
                    broker->sendPublish(clientId, message, clientQoS, false);
                }

                LOG(DEBUG) << "MQTT Broker:   ... completed!";
            }
        }
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId, std::string topic) {
        if (topic.empty()) {
            if (clientIds.erase(clientId) != 0) {
                LOG(DEBUG) << "MQTT Broker: Unsubscribe";
                LOG(DEBUG) << "MQTT Broker:   ClientId: " << clientId;
                LOG(DEBUG) << "MQTT Broker:   Topic: " << topicLevel;
            }
        } else {
            const std::string topicLevel = topic.substr(0, topic.find('/')); // cppcheck-suppress shadowVariable

            auto&& it = topicLevels.find(topicLevel);
            if (it != topicLevels.end()) {
                topic.erase(0, topicLevel.size() + 1);

                if (it->second.unsubscribe(clientId, topic)) {
                    LOG(DEBUG) << "MQTT Broker:   Erase Topic: " << it->first;

                    topicLevels.erase(it);
                }
            }
        }

        return clientIds.empty() && topicLevels.empty();
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId) {
        if (clientIds.erase(clientId) != 0) {
            LOG(DEBUG) << "MQTT Broker: Unsubscribe";
            LOG(DEBUG) << "MQTT Broker:   ClientId: " << clientId;
            LOG(DEBUG) << "MQTT Broker:   Topic: " << topicLevel;
        }

        for (auto it = topicLevels.begin(); it != topicLevels.end();) {
            if (it->second.unsubscribe(clientId)) {
                LOG(DEBUG) << "MQTT Broker:   Erase Topic: " << it->first;

                it = topicLevels.erase(it);
            } else {
                ++it;
            }
        }

        return clientIds.empty() && topicLevels.empty();
    }

    nlohmann::json SubscribtionTree::TopicLevel::toJson() const {
        nlohmann::json json;

        for (const auto& [topicLevelName, topicLevel] : topicLevels) { // cppcheck-suppress shadowVariable
            json["topic_filter"][topicLevelName] = topicLevel.toJson();
        }

        for (const auto& [subscriber, qoS] : clientIds) {
            json["qos_map"][subscriber] = qoS;
        }

        return json;
    }

    SubscribtionTree::TopicLevel& SubscribtionTree::TopicLevel::fromJson(const nlohmann::json& json) {
        clientIds.clear();
        topicLevels.clear();

        if (json.contains("qos_map")) {
            for (const auto& subscriber : json["qos_map"].items()) {
                clientIds.emplace(subscriber.key(), subscriber.value());
            }
        }

        if (json.contains("topic_filter")) {
            for (const auto& topicLevelItem : json["topic_filter"].items()) {
                topicLevels.emplace(topicLevelItem.key(), TopicLevel(broker, topicLevelItem.key()).fromJson(topicLevelItem.value()));
            }
        }

        return *this;
    }

    void SubscribtionTree::TopicLevel::clear() {
        *this = TopicLevel(broker, "");
    }

    void SubscribtionTree::fromJson(const nlohmann::json& json) {
        if (!json.empty()) {
            head.fromJson(json);
        }
    }

    void SubscribtionTree::clear() {
        head.clear();
    }

    nlohmann::json SubscribtionTree::toJson() const {
        return head.toJson();
    }

} // namespace iot::mqtt::server::broker
