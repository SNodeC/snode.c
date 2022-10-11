/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

    void RetainTree::retain(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel) {
        head.retain(fullTopicName, message, qoSLevel, fullTopicName, false);
    }

    void RetainTree::publish(std::string subscribedTopicName, const std::string& clientId, uint8_t clientQoSLevel) {
        head.publish(clientId, clientQoSLevel, subscribedTopicName, false);
    }

    RetainTree::RetainTreeNode::RetainTreeNode(iot::mqtt::server::broker::Broker* broker)
        : broker(broker) {
    }

    bool RetainTree::RetainTreeNode::retain(
        const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, std::string remainingTopicName, bool leafFound) {
        if (leafFound) {
            if (!fullTopicName.empty()) {
                LOG(TRACE) << "Retaining: " << fullTopicName << " - " << message;
                this->fullTopicName = fullTopicName;
                this->message = message;
                this->qoSLevel = qoSLevel;
            }
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTreeNodes.insert({topicName, RetainTreeNode(broker)})
                    .first->second.retain(fullTopicName, message, qoSLevel, remainingTopicName, leafFound)) {
                topicTreeNodes.erase(topicName);
            }
        }

        return this->message.empty() && topicTreeNodes.empty();
    }

    void RetainTree::RetainTreeNode::publish(const std::string& clientId,
                                             uint8_t clientQoSLevel,
                                             std::string remainingSubscribedTopicName,
                                             bool leafFound) {
        if (leafFound) {
            if (!fullTopicName.empty()) {
                LOG(TRACE) << "Found retained message: " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
                LOG(TRACE) << "Distribute message ...";
                broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, RETAIN_TRUE, clientQoSLevel);
                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingSubscribedTopicName.find('/');

            std::string topicName = remainingSubscribedTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingSubscribedTopicName.erase(0, topicName.size() + 1);

            auto foundNode = topicTreeNodes.find(topicName);
            if (foundNode != topicTreeNodes.end()) {
                foundNode->second.publish(clientId, clientQoSLevel, remainingSubscribedTopicName, leafFound);
            } else if (topicName == "+") {
                for (auto& [notUsed, topicTree] : topicTreeNodes) {
                    topicTree.publish(clientId, clientQoSLevel, remainingSubscribedTopicName, leafFound);
                }
            } else if (topicName == "#") {
                publish(clientId, clientQoSLevel);
            }
        }
    }

    void RetainTree::RetainTreeNode::publish(const std::string& clientId, uint8_t clientQoSLevel) {
        if (!fullTopicName.empty()) {
            LOG(TRACE) << "Found retained message: " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
            LOG(TRACE) << "Distribute message ...";
            broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, RETAIN_TRUE, clientQoSLevel);
            LOG(TRACE) << "... completed!";
        }

        for (auto& [topicName, topicTree] : topicTreeNodes) {
            topicTree.publish(clientId, clientQoSLevel);
        }
    }

} // namespace iot::mqtt::server::broker
