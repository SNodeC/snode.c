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

#include <utility>

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

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTreeNodes.insert({topicName, RetainTreeNode(broker)}).first->second.retain(message, remainingTopicName, leafFound)) {
                topicTreeNodes.erase(topicName);
            }
        }

        return this->message.getMessage().empty() && topicTreeNodes.empty();
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

            std::string topicName = remainingSubscribedTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingSubscribedTopicName.erase(0, topicName.size() + 1);

            auto foundNode = topicTreeNodes.find(topicName);
            if (foundNode != topicTreeNodes.end()) {
                foundNode->second.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
            } else if (topicName == "+") {
                for (auto& [notUsed, topicTree] : topicTreeNodes) {
                    topicTree.publish(clientId, clientQoS, remainingSubscribedTopicName, leafFound);
                }
            } else if (topicName == "#") {
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

        for (auto& [topicName, topicTree] : topicTreeNodes) {
            topicTree.publish(clientId, clientQoS);
        }
    }

} // namespace iot::mqtt::server::broker
