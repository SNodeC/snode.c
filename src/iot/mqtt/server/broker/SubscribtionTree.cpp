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
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>
// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    SubscribtionTree::SubscribtionTree(iot::mqtt::server::broker::Broker* broker)
        : head(broker) {
    }

    void SubscribtionTree::appear(const std::string& clientId) {
        head.appear(clientId, "");
    }

    bool SubscribtionTree::subscribe(const std::string& topic, const std::string& clientId, uint8_t qoS) {
        return head.subscribe(clientId, qoS, topic, false);
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

    void SubscribtionTree::TopicLevel::appear(const std::string& clientId, const std::string& topic) {
        if (subscribers.contains(clientId)) {
            broker->appear(clientId, topic, subscribers[clientId]);
        }

        for (auto& [topicLevel, subscribtion] : topicLevels) {
            subscribtion.appear(clientId, topic + (topic.empty() ? "" : "/") + topicLevel);
        }
    }

    bool SubscribtionTree::TopicLevel::subscribe(const std::string& clientId, uint8_t qoS, std::string topic, bool leafFound) {
        bool success = true;

        if (leafFound) {
            subscribers[clientId] = qoS;
        } else {
            std::string::size_type slashPosition = topic.find('/');

            std::string topicLevel = topic.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            if ((topicLevel == "#" && !topic.ends_with('#'))) {
                success = false;
            } else {
                topic.erase(0, topicLevel.size() + 1);

                success = topicLevels.insert({topicLevel, SubscribtionTree::TopicLevel(broker)})
                              .first->second.subscribe(clientId, qoS, topic, leafFound);
            }
        }

        return success;
    }

    void SubscribtionTree::TopicLevel::publish(Message& message, std::string topic, bool leafFound) {
        if (leafFound) {
            LOG(TRACE) << "Found match:";
            LOG(TRACE) << "  Topic: '" << message.getTopic() << "';";
            LOG(TRACE) << "  Message: '" << message.getMessage() << "' ";
            LOG(TRACE) << "Distribute Publish for match ...";

            for (auto& [clientId, clientQoS] : subscribers) {
                broker->sendPublish(clientId, message, clientQoS, false);
            }

            LOG(TRACE) << "... completed!";

            auto nextHashLevel = topicLevels.find("#");
            if (nextHashLevel != topicLevels.end()) {
                LOG(TRACE) << "Found parent match:";
                LOG(TRACE) << "  Topic: '" << message.getTopic() << "'";
                LOG(TRACE) << "  Message: '" << message.getMessage() << "'";
                LOG(TRACE) << "Distribute Publish for match ...";

                for (auto& [clientId, clientQoS] : nextHashLevel->second.subscribers) {
                    broker->sendPublish(clientId, message, clientQoS, false);
                }

                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string::size_type slashPosition = topic.find('/');

            std::string topicLevel = topic.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            topic.erase(0, topicLevel.size() + 1);

            auto foundNode = topicLevels.find(topicLevel);
            if (foundNode != topicLevels.end()) {
                foundNode->second.publish(message, topic, leafFound);
            }

            foundNode = topicLevels.find("+");
            if (foundNode != topicLevels.end()) {
                foundNode->second.publish(message, topic, leafFound);
            }

            foundNode = topicLevels.find("#");
            if (foundNode != topicLevels.end()) {
                LOG(TRACE) << "Found match for topic filter: '.../" << topicLevel << "/#', topic: '" << message.getTopic()
                           << "', Message: '" << message.getMessage() << "'";
                LOG(TRACE) << "Distribute Publish ...";
                for (auto& [clientId, clientQoS] : foundNode->second.subscribers) {
                    broker->sendPublish(clientId, message, clientQoS, false);
                }
                LOG(TRACE) << "... completed!";
            }
        }
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId, std::string topic, bool leafFound) {
        if (leafFound) {
            subscribers.erase(clientId);
        } else {
            std::string::size_type slashPosition = topic.find('/');

            std::string topicLevel = topic.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            topic.erase(0, topicLevel.size() + 1);

            if (topicLevels.contains(topicLevel) && topicLevels.find(topicLevel)->second.unsubscribe(clientId, topic, leafFound)) {
                topicLevels.erase(topicLevel);
            }
        }

        return subscribers.empty() && topicLevels.empty();
    }

    bool SubscribtionTree::TopicLevel::unsubscribe(const std::string& clientId) {
        subscribers.erase(clientId);

        for (auto it = topicLevels.begin(); it != topicLevels.end();) {
            if (it->second.unsubscribe(clientId)) {
                it = topicLevels.erase(it);
            } else {
                ++it;
            }
        }

        return subscribers.empty() && topicLevels.empty();
    }

    nlohmann::json SubscribtionTree::TopicLevel::toJson() const {
        nlohmann::json json;

        for (auto& [topicLevelName, topicLevel] : topicLevels) {
            json["topic_filter"][topicLevelName] = topicLevel.toJson();
        }

        for (const auto& [subscriber, qoS] : subscribers) {
            json["subscribers"][subscriber] = qoS;
        }

        return json;
    }

    SubscribtionTree::TopicLevel& SubscribtionTree::TopicLevel::fromJson(const nlohmann::json& json) {
        subscribers.clear();
        topicLevels.clear();

        if (json.contains("subscribers")) {
            for (const auto& subscriber : json["subscribers"].items()) {
                subscribers.emplace(subscriber.key(), subscriber.value());
            }
        }

        if (json.contains("topic_filter")) {
            for (const auto& topicLevelItem : json["topic_filter"].items()) {
                topicLevels.emplace(topicLevelItem.key(), TopicLevel(broker).fromJson(topicLevelItem.value()));
            }
        }

        return *this;
    }

    void SubscribtionTree::TopicLevel::clear() {
        *this = TopicLevel(broker);
    }

    void SubscribtionTree::fromJson(const nlohmann::json& json) {
        if (!json.empty()) {
            head.fromJson(json);
        }
    }

    nlohmann::json SubscribtionTree::toJson() const {
        return head.toJson();
    }

} // namespace iot::mqtt::server::broker
