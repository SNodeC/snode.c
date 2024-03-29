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

#include "iot/mqtt/server/broker/RetainTree.h"

#include "iot/mqtt/Mqtt.h"
#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    RetainTree::RetainTree(Broker* broker)
        : head(broker) {
    }

    void RetainTree::retain(Message&& message) {
        head.retain(message, message.getTopic(), false);
    }

    void RetainTree::appear(const std::string& clientId, const std::string& topic, uint8_t qoS) {
        head.appear(clientId, topic, qoS);
    }

    RetainTree::TopicLevel::TopicLevel(iot::mqtt::server::broker::Broker* broker)
        : broker(broker) {
    }

    RetainTree::TopicLevel& RetainTree::TopicLevel::fromJson(const nlohmann::json& json) {
        subTopicLevels.clear();

        message.fromJson(json.value("message", nlohmann::json()));

        if (json.contains("topic_level")) {
            for (const auto& topicLevelItem : json["topic_level"].items()) {
                subTopicLevels.emplace(topicLevelItem.key(), TopicLevel(broker).fromJson(topicLevelItem.value()));
            }
        }

        return *this;
    }

    void RetainTree::fromJson(const nlohmann::json& json) {
        if (!json.empty()) {
            head.fromJson(json);
        }
    }

    nlohmann::json RetainTree::toJson() const {
        return head.toJson();
    }

    void RetainTree::clear() {
        head.clear();
    }

    bool RetainTree::TopicLevel::retain(const Message& message, std::string topic, bool leafFound) {
        if (leafFound) {
            if (!message.getTopic().empty()) {
                LOG(DEBUG) << "MQTT Broker: Retaining:";
                LOG(DEBUG) << "MQTT Broker:   Topic: " << message.getTopic();
                LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
                LOG(DEBUG) << "MQTT Broker:     QoS: " << static_cast<uint16_t>(message.getQoS());
                this->message = message;
            }
        } else {
            const std::string::size_type slashPosition = topic.find('/');
            const std::string topicLevel = topic.substr(0, slashPosition);

            topic.erase(0, topicLevel.size() + 1);

            if (subTopicLevels.insert({topicLevel, TopicLevel(broker)})
                    .first->second.retain(message, topic, slashPosition == std::string::npos)) {
                subTopicLevels.erase(topicLevel);
            }
        }

        return this->message.getMessage().empty() && subTopicLevels.empty();
    }

    void RetainTree::TopicLevel::appear(const std::string& clientId, const std::string& topic, uint8_t qoS) {
        appear(clientId, topic, qoS, false);
    }

    void RetainTree::TopicLevel::appear(const std::string& clientId, std::string topic, uint8_t qoS, bool appeared) {
        if (appeared) {
            if (!message.getTopic().empty()) {
                LOG(DEBUG) << "MQTT Broker: Retained Topic found:";
                LOG(DEBUG) << "MQTT Broker:   Topic: " << message.getTopic();
                LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
                LOG(DEBUG) << "MQTT Broker:     QoS: " << static_cast<uint16_t>(message.getQoS());
                LOG(DEBUG) << "MQTT Broker:   Client:";
                LOG(DEBUG) << "MQTT Broker:     QoS: " << static_cast<uint16_t>(qoS);
                LOG(DEBUG) << "MQTT Broker:   distributing message ...";
                broker->sendPublish(clientId, message, std::min(message.getQoS(), qoS), true);
                LOG(DEBUG) << "MQTT Broker:   ... completed!";
            }
        } else {
            const std::string::size_type slashPosition = topic.find('/');
            const std::string topicLevel = topic.substr(0, slashPosition);
            const bool leafFound = slashPosition == std::string::npos;

            topic.erase(0, topicLevel.size() + 1);

            auto foundNode = subTopicLevels.find(topicLevel);
            if (foundNode != subTopicLevels.end()) {
                foundNode->second.appear(clientId, topic, qoS, leafFound);
            } else if (topicLevel == "+") {
                for (auto& [notUsed, topicTree] : subTopicLevels) {
                    topicTree.appear(clientId, topic, qoS, leafFound);
                }
            } else if (topicLevel == "#") {
                appear(clientId, qoS);
            }
        }
    }

    void RetainTree::TopicLevel::appear(const std::string& clientId, uint8_t clientQoS) {
        if (!message.getTopic().empty()) {
            LOG(DEBUG) << "MQTT Broker: Retained Topic found:";
            LOG(DEBUG) << "MQTT Broker:   Topic: " << message.getTopic();
            LOG(DEBUG) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
            LOG(DEBUG) << "MQTT Broker:     QoS: " << static_cast<uint16_t>(message.getQoS());
            LOG(DEBUG) << "MQTT Broker:   Client:";
            LOG(DEBUG) << "MQTT Broker:     QoS: " << static_cast<uint16_t>(clientQoS);
            LOG(DEBUG) << "MQTT Broker:   distributing message ...";
            broker->sendPublish(clientId, message, std::min(message.getQoS(), clientQoS), true);
            LOG(DEBUG) << "MQTT Broker:   ... completed!";
        }

        for (auto& [topicLevel, topicTree] : subTopicLevels) {
            topicTree.appear(clientId, clientQoS);
        }
    }

    nlohmann::json RetainTree::TopicLevel::toJson() const {
        nlohmann::json json;

        if (!message.getMessage().empty()) {
            json["message"] = message.toJson();
        }

        for (const auto& [topicLevel, topicLevelValue] : subTopicLevels) {
            json["topic_level"][topicLevel] = topicLevelValue.toJson();
        }

        return json;
    }

    void RetainTree::TopicLevel::clear() {
        *this = TopicLevel(broker);
    }

} // namespace iot::mqtt::server::broker
