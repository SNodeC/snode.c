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

#include "iot/mqtt/server/broker/SubscribtionTree.h"

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    SubscribtionTree::SubscribtionTree(iot::mqtt::server::broker::Broker* broker)
        : head(broker) {
    }

    void SubscribtionTree::publishRetained(const std::string& clientId) {
        head.publishRetained(clientId);
    }

    bool SubscribtionTree::subscribe(const std::string& fullTopicName, const std::string& clientId, uint8_t clientQoSLevel) {
        return head.subscribe(fullTopicName, clientId, clientQoSLevel, fullTopicName, false);
    }

    void SubscribtionTree::publish(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, bool retained) {
        head.publish(fullTopicName, message, qoSLevel, retained, fullTopicName, false);
    }

    bool SubscribtionTree::unsubscribe(std::string fullTopicName, const std::string& clientId) {
        return head.unsubscribe(clientId, fullTopicName, false);
    }

    bool SubscribtionTree::unsubscribe(const std::string& clientId) {
        return head.unsubscribe(clientId);
    }

    SubscribtionTree::SubscribtionTreeNode::SubscribtionTreeNode(Broker* broker)
        : broker(broker) {
    }

    void SubscribtionTree::SubscribtionTreeNode::publishRetained(const std::string& clientId) {
        if (subscribers.contains(clientId) && !subscribedTopicName.empty()) {
            broker->publishRetained(subscribedTopicName, clientId, subscribers[clientId]);
        }

        for (auto& [topicName, subscribtion] : subscribtions) {
            subscribtion.publishRetained(clientId);
        }
    }

    bool SubscribtionTree::SubscribtionTreeNode::subscribe(const std::string& fullTopicName,
                                                           const std::string& clientId,
                                                           uint8_t clientQoSLevel,
                                                           std::string remainingTopicName,
                                                           bool leafFound) {
        bool success = true;

        if (leafFound) {
            subscribedTopicName = fullTopicName;
            subscribers[clientId] = clientQoSLevel;
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            if (topicName == "#" && !remainingTopicName.ends_with("#")) {
                success = false;
            } else {
                remainingTopicName.erase(0, topicName.size() + 1);

                success = subscribtions.insert({topicName, SubscribtionTree::SubscribtionTreeNode(broker)})
                              .first->second.subscribe(fullTopicName, clientId, clientQoSLevel, remainingTopicName, leafFound);
            }
        }

        return success;
    }

    void SubscribtionTree::SubscribtionTreeNode::publish(const std::string& fullTopicName,
                                                         const std::string& message,
                                                         uint8_t qoSLevel,
                                                         bool retained,
                                                         std::string remainingTopicName,
                                                         bool leafFound) {
        if (leafFound) {
            if (!subscribedTopicName.empty()) {
                LOG(TRACE) << "Found match:";
                LOG(TRACE) << "  Received topic = '" << fullTopicName << "';";
                LOG(TRACE) << "  Matched topic = '" << subscribedTopicName << "'";

                LOG(TRACE) << "  Message:' " << message << "' ";
                LOG(TRACE) << "Distribute Publish ...";

                for (auto& [clientId, clientQoSLevel] : subscribers) {
                    broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                }

                LOG(TRACE) << "... completed!";
            }

            auto nextHashNode = subscribtions.find("#");
            if (nextHashNode != subscribtions.end()) {
                LOG(TRACE) << "Found parent match:";
                LOG(TRACE) << "  Received topic = '" << fullTopicName << "'";
                LOG(TRACE) << "  Matched topic : '" << fullTopicName << "/#'";
                LOG(TRACE) << "  Message: '" << message << "'";
                LOG(TRACE) << "Distribute Publish ...";

                for (auto& [clientId, clientQoSLevel] : nextHashNode->second.subscribers) {
                    broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                }

                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            auto foundNode = subscribtions.find(topicName);
            if (foundNode != subscribtions.end()) {
                foundNode->second.publish(fullTopicName, message, qoSLevel, retained, remainingTopicName, leafFound);
            }

            foundNode = subscribtions.find("+");
            if (foundNode != subscribtions.end()) {
                foundNode->second.publish(fullTopicName, message, qoSLevel, retained, remainingTopicName, leafFound);
            }

            foundNode = subscribtions.find("#");
            if (foundNode != subscribtions.end()) {
                LOG(TRACE) << "Found match: Subscribed topic: '" << foundNode->second.subscribedTopicName << "', topic: '" << fullTopicName
                           << "', Message: '" << message << "'";
                LOG(TRACE) << "Distribute Publish ...";
                for (auto& [clientId, clientQoSLevel] : foundNode->second.subscribers) {
                    broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                }
                LOG(TRACE) << "... completed!";
            }
        }
    }

    bool SubscribtionTree::SubscribtionTreeNode::unsubscribe(const std::string& clientId, std::string remainingTopicName, bool leafFound) {
        if (leafFound) {
            subscribers.erase(clientId);
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName) &&
                subscribtions.find(topicName)->second.unsubscribe(clientId, remainingTopicName, leafFound)) {
                subscribtions.erase(topicName);
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    bool SubscribtionTree::SubscribtionTreeNode::unsubscribe(const std::string& clientId) {
        subscribers.erase(clientId);

        for (auto it = subscribtions.begin(); it != subscribtions.end();) {
            if (it->second.unsubscribe(clientId)) {
                it = subscribtions.erase(it);
            } else {
                ++it;
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

} // namespace iot::mqtt::server::broker
