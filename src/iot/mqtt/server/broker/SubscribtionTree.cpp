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

#include "iot/mqtt/server/broker/SubscribtionTree.h"

#include "iot/mqtt/server/broker/Broker.h"
#include "iot/mqtt/server/broker/Message.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>
// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    SubscribtionTree::SubscribtionTree(iot::mqtt::server::broker::Broker* broker)
        : head(broker) {
    }

    void SubscribtionTree::publishRetained(const std::string& clientId) {
        head.publishRetained(clientId);
    }

    bool SubscribtionTree::subscribe(const std::string& topic, const std::string& clientId, uint8_t clientQoS) {
        return head.subscribe(topic, clientId, clientQoS, topic, false);
    }

    void SubscribtionTree::publish(Message&& message) {
        head.publish(message, message.getTopic(), false);
    }

    bool SubscribtionTree::unsubscribe(std::string topic, const std::string& clientId) {
        return head.unsubscribe(clientId, topic, false);
    }

    bool SubscribtionTree::unsubscribe(const std::string& clientId) {
        return head.unsubscribe(clientId);
    }

    SubscribtionTree::TopicLevel::TopicLevel(Broker* broker)
        : broker(broker) {
    }

    void SubscribtionTree::TopicLevel::publishRetained(const std::string& clientId) {
        if (subscribers.contains(clientId) && !subscribedTopicName.empty()) {
            broker->publishRetainedMessage(clientId, subscribedTopicName, subscribers[clientId]);
        }

        for (auto& [topic, subscribtion] : subTopicLevels) {
            subscribtion.publishRetained(clientId);
        }
    }

    bool SubscribtionTree::TopicLevel::subscribe(
        const std::string& topic, const std::string& clientId, uint8_t clientQoS, std::string remainingTopicName, bool leafFound) {
        bool success = true;

        if (leafFound) {
            subscribedTopicName = topic;
            subscribers[clientId] = clientQoS;
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topic = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            if (topic == "#" && !remainingTopicName.ends_with("#")) {
                success = false;
            } else {
                remainingTopicName.erase(0, topic.size() + 1);

                success = subTopicLevels.insert({topic, SubscribtionTree::TopicLevel(broker)})
                              .first->second.subscribe(topic, clientId, clientQoS, remainingTopicName, leafFound);
            }
        }

        return success;
    }

    void SubscribtionTree::TopicLevel::publish(Message& message, std::string remainingTopicName, bool leafFound) {
        if (leafFound) {
            if (!subscribedTopicName.empty()) {
                LOG(TRACE) << "Found match:";
                LOG(TRACE) << "  Topic: '" << message.getTopic() << "';";
                LOG(TRACE) << "  Filter: '" << subscribedTopicName << "'";

                LOG(TRACE) << "  Message: '" << message.getMessage() << "' ";
                LOG(TRACE) << "Distribute Publish for match ...";

                for (auto& [clientId, clientQoS] : subscribers) {
                    broker->sendPublish(clientId, message, clientQoS);
                }

                LOG(TRACE) << "... completed!";
            }

            auto nextHashNode = subTopicLevels.find("#");
            if (nextHashNode != subTopicLevels.end()) {
                LOG(TRACE) << "Found parent match:";
                LOG(TRACE) << "  Topic: '" << message.getTopic() << "'";
                LOG(TRACE) << "  Filter: '" << message.getTopic() << "/#'";
                LOG(TRACE) << "  Message: '" << message.getMessage() << "'";
                LOG(TRACE) << "Distribute Publish for math ...";

                for (auto& [clientId, clientQoS] : nextHashNode->second.subscribers) {
                    broker->sendPublish(clientId, message, clientQoS);
                }

                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topic = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topic.size() + 1);

            auto foundNode = subTopicLevels.find(topic);
            if (foundNode != subTopicLevels.end()) {
                foundNode->second.publish(message, remainingTopicName, leafFound);
            }

            foundNode = subTopicLevels.find("+");
            if (foundNode != subTopicLevels.end()) {
                foundNode->second.publish(message, remainingTopicName, leafFound);
            }

            foundNode = subTopicLevels.find("#");
            if (foundNode != subTopicLevels.end()) {
                LOG(TRACE) << "Found match: Subscribed topic: '" << foundNode->second.subscribedTopicName << "', topic: '"
                           << message.getTopic() << "', Message: '" << message.getMessage() << "'";
                LOG(TRACE) << "Distribute Publish ...";
                for (auto& [clientId, clientQoS] : foundNode->second.subscribers) {
                    broker->sendPublish(clientId, message, clientQoS);
                }
                LOG(TRACE) << "... completed!";
            }
        }
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId, std::string remainingTopicName, bool leafFound) {
        if (leafFound) {
            subscribers.erase(clientId);
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topic = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topic.size() + 1);

            if (subTopicLevels.contains(topic) && subTopicLevels.find(topic)->second.unsubscribe(clientId, remainingTopicName, leafFound)) {
                subTopicLevels.erase(topic);
            }
        }

        return subscribers.empty() && subTopicLevels.empty();
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId) {
        subscribers.erase(clientId);

        for (auto it = subTopicLevels.begin(); it != subTopicLevels.end();) {
            if (it->second.unsubscribe(clientId)) {
                it = subTopicLevels.erase(it);
            } else {
                ++it;
            }
        }

        return subscribers.empty() && subTopicLevels.empty();
    }

    nlohmann::json SubscribtionTree::TopicLevel::toJson() const {
        nlohmann::json json;

        json["subscribed_topic"] = subscribedTopicName;

        for (auto& [topicLevel, topicLevelValue] : subTopicLevels) {
            json["topic_level"][topicLevel] = topicLevelValue.toJson();
        }

        for (auto& [subscribers, qoS] : subscribers) {
            json["subscribers"][subscribers] = qoS;
        }

        return json;
    }

    SubscribtionTree::TopicLevel& SubscribtionTree::TopicLevel::fromJson(const nlohmann::json& json) {
        subscribedTopicName = json.value("subscribed_topic", "");

        if (json.contains("subscribers")) {
            for (auto& subscriber : json["subscribers"].items()) {
                subscribers.emplace(subscriber.key(), subscriber.value());
            }
        }

        if (json.contains("topic_level")) {
            for (auto& topicLevelItem : json["topic_level"].items()) {
                subTopicLevels.emplace(topicLevelItem.key(), TopicLevel(broker).fromJson(topicLevelItem.value()));
            };
        }

        return *this;
    }

    void SubscribtionTree::fromJson(const nlohmann::json& json) {
        head.subTopicLevels.clear();

        head.fromJson(json);
    }

    nlohmann::json SubscribtionTree::toJson() const {
        nlohmann::json json = head.toJson();

        return json;
    }

} // namespace iot::mqtt::server::broker
