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
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <bits/utility.h>
// IWYU pragma: no_include <type_traits>

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

    bool RetainTree::TopicLevel::retain(const Message& message, std::string topic, bool appeared) {
        if (appeared) {
            if (!message.getTopic().empty()) {
                LOG(TRACE) << "Retaining: " << message.getTopic() << " - " << message.getMessage();
                this->message = message;
            }
        } else {
            std::string::size_type slashPosition = topic.find('/');

            std::string topicLevel = topic.substr(0, slashPosition);
            bool appeared = slashPosition == std::string::npos;

            topic.erase(0, topicLevel.size() + 1);

            if (subTopicLevels.insert({topicLevel, TopicLevel(broker)}).first->second.retain(message, topic, appeared)) {
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
            if (!message.getMessage().empty()) {
                LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                           << static_cast<uint16_t>(message.getQoS());
                LOG(TRACE) << "  distribute message ...";
                broker->sendPublish(clientId, message, qoS, true);
                LOG(TRACE) << "  ... completed!";
            }
        } else {
            std::string::size_type slashPosition = topic.find('/');

            std::string topicLevel = topic.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

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
            LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                       << static_cast<uint16_t>(message.getQoS());
            LOG(TRACE) << "  distribute message ...";
            broker->sendPublish(clientId, message, clientQoS, true);
            LOG(TRACE) << "  ... completed!";
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

        for (auto& [topicLevel, topicLevelValue] : subTopicLevels) {
            json["topic_level"][topicLevel] = topicLevelValue.toJson();
        }

        return json;
    }

    void RetainTree::TopicLevel::clear() {
        *this = TopicLevel(broker);
    }

} // namespace iot::mqtt::server::broker
