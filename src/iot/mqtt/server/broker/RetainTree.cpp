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

    void RetainTree::publish(std::string subscribedTopicName, const std::string& clientId, uint8_t clientQoS) {
        head.publish(clientId, clientQoS, subscribedTopicName, false);
    }

    RetainTree::RetainTreeNode::RetainTreeNode(iot::mqtt::server::broker::Broker* broker)
        : broker(broker) {
    }

    bool RetainTree::RetainTreeNode::retain(Message& message, std::string remainingTopicName, bool leafFound) {
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

            if (retainTreeNodes.insert({topicLevel, RetainTreeNode(broker)}).first->second.retain(message, remainingTopicName, leafFound)) {
                retainTreeNodes.erase(topicLevel);
            }
        }

        return this->message.getMessage().empty() && retainTreeNodes.empty();
    }

    void RetainTree::RetainTreeNode::publish(const std::string& clientId,
                                             uint8_t clientQoS,
                                             std::string remainingSubscribedTopicName,
                                             bool leafFound) {
        if (leafFound) {
            if (!message.getMessage().empty()) {
                LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                           << static_cast<uint16_t>(message.getQoS());
                LOG(TRACE) << "  distribute message ...";
                broker->sendPublish(clientId, message, clientQoS);
                LOG(TRACE) << "  ... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingSubscribedTopicName.find('/');

            std::string topicLevel = remainingSubscribedTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingSubscribedTopicName.erase(0, topicLevel.size() + 1);

            auto foundNode = retainTreeNodes.find(topicLevel);
            if (foundNode != retainTreeNodes.end()) {
                foundNode->second.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
            } else if (topicLevel == "+") {
                for (auto& [notUsed, topicTree] : retainTreeNodes) {
                    topicTree.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
                }
            } else if (topicLevel == "#") {
                publish(clientId, clientQoS);
            }
        }
    }

    void RetainTree::RetainTreeNode::publish(const std::string& clientId, uint8_t clientQoS) {
        if (!message.getTopic().empty()) {
            LOG(TRACE) << "Retained message found: " << message.getTopic() << " - " << message.getMessage() << " - "
                       << static_cast<uint16_t>(message.getQoS());
            LOG(TRACE) << "  distribute message ...";
            broker->sendPublish(clientId, message, clientQoS);
            LOG(TRACE) << "  ... completed!";
        }

        for (auto& [topicLevel, topicTree] : retainTreeNodes) {
            topicTree.publish(clientId, clientQoS);
        }
    }

    nlohmann::json RetainTree::RetainTreeNode::toJson() const {
        nlohmann::json json;

        if (!message.getMessage().empty()) {
            json["message"] = message.toJson();
        }

        for (auto& [topicLevel, retainTreeNode] : retainTreeNodes) {
            json["topic_level"][topicLevel] = retainTreeNode.toJson();
        }

        return json;
    }

    void RetainTree::RetainTreeNode::fromJson(const nlohmann::json& json) {
        if (json.contains("message")) {
            this->message = Message(json["message"]);
        }

        if (json.contains("topic_level")) {
            for (auto& topicLevelItem : json["topic_level"].items()) {
                RetainTreeNode subTree(broker);

                subTree.fromJson(topicLevelItem.value());

                retainTreeNodes.emplace(topicLevelItem.key(), subTree);
            };
        }
    }

    void RetainTree::fromJson(const nlohmann::json& json) {
        head.fromJson(json);
    }

    nlohmann::json RetainTree::toJson() const {
        nlohmann::json json = head.toJson();

        return json;
    }

} // namespace iot::mqtt::server::broker
