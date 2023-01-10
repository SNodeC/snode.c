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

#include "iot/mqtt/server/broker/RetainTree.h"

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <nlohmann/json.hpp>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    RetainTree::RetainTree(Broker* broker)
        : head(broker) {
    }

    void RetainTree::retain(Message&& message) {
        head.retain(message, message.getTopic(), false);
    }

    void RetainTree::publish(const std::string& clientId, uint8_t qoS, const std::string& topic) {
        head.publish(clientId, qoS, topic);
    }

    RetainTree::TopicLevel::TopicLevel(iot::mqtt::server::broker::Broker* broker)
        : broker(broker) {
    }

    bool RetainTree::TopicLevel::retain(Message& message, std::string remainingTopicName, bool leafFound) {
        if (leafFound) {
            if (!message.getTopic().empty()) {
                LOG(TRACE) << "Retaining: " << message.getTopic() << " - " << message.getMessage();
                this->message = message;
            }
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicLevel = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicLevel.size() + 1);

            if (subTopicLevels.insert({topicLevel, TopicLevel(broker)}).first->second.retain(message, remainingTopicName, leafFound)) {
                subTopicLevels.erase(topicLevel);
            }
        }

        return this->message.getMessage().empty() && subTopicLevels.empty();
    }

    void RetainTree::TopicLevel::publish(const std::string& clientId, uint8_t clientQoS, const std::string& topic) {
        publish(clientId, clientQoS, topic, false);
    }

    void RetainTree::TopicLevel::publish(const std::string& clientId,
                                         uint8_t clientQoS,
                                         std::string remainingSubscribedTopicName,
                                         bool leafFound) {
        if (leafFound) {
            if (!message.getMessage().empty()) {
                LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                           << static_cast<uint16_t>(message.getQoS());
                LOG(TRACE) << "  distribute message ...";
                broker->sendPublish(clientId, message, clientQoS, true);
                LOG(TRACE) << "  ... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingSubscribedTopicName.find('/');

            std::string topicLevel = remainingSubscribedTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingSubscribedTopicName.erase(0, topicLevel.size() + 1);

            auto foundNode = subTopicLevels.find(topicLevel);
            if (foundNode != subTopicLevels.end()) {
                foundNode->second.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
            } else if (topicLevel == "+") {
                for (auto& [notUsed, topicTree] : subTopicLevels) {
                    topicTree.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
                }
            } else if (topicLevel == "#") {
                publish(clientId, clientQoS);
            }
        }
    }

    void RetainTree::TopicLevel::publish(const std::string& clientId, uint8_t clientQoS) {
        if (!message.getTopic().empty()) {
            LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                       << static_cast<uint16_t>(message.getQoS());
            LOG(TRACE) << "  distribute message ...";
            broker->sendPublish(clientId, message, clientQoS, true);
            LOG(TRACE) << "  ... completed!";
        }

        for (auto& [topicLevel, topicTree] : subTopicLevels) {
            topicTree.publish(clientId, clientQoS);
        }
    }

    nlohmann::json RetainTree::TopicLevel::toJson() const {
        nlohmann::json retainTreeJson;

        if (!message.getMessage().empty()) {
            retainTreeJson["message"] = message.toJson();
        }

        for (auto& [topicLevel, topicLevelValue] : subTopicLevels) {
            retainTreeJson["topic_level"][topicLevel] = topicLevelValue.toJson();
        }

        return retainTreeJson;
    }

    RetainTree::TopicLevel& RetainTree::TopicLevel::fromJson(const nlohmann::json& retainTreeJson) {
        subTopicLevels.clear();

        message.fromJson(retainTreeJson.value("message", nlohmann::json()));

        if (retainTreeJson.contains("topic_level")) {
            for (auto& topicLevelItem : retainTreeJson["topic_level"].items()) {
                subTopicLevels.emplace(topicLevelItem.key(), TopicLevel(broker).fromJson(topicLevelItem.value()));
            }
        }

        return *this;
    }

    void RetainTree::fromJson(const nlohmann::json& retainTreeJson) {
        if (!retainTreeJson.empty()) {
            head.fromJson(retainTreeJson);
        }
    }

    nlohmann::json RetainTree::toJson() const {
        return head.toJson();
    }

} // namespace iot::mqtt::server::broker
