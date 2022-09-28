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

#include "broker/SubscribtionTree.h"

#include "broker/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    void SubscribtionTree::subscribe(const std::string& fullTopicName, mqtt::broker::SocketContext* socketContext, uint8_t qoSLevel) {
        subscribe(fullTopicName, fullTopicName, socketContext, qoSLevel);
    }

    void SubscribtionTree::publish(const std::string& fullTopicName, const std::string& message) {
        publish(fullTopicName, fullTopicName, message);
    }

    bool SubscribtionTree::unsubscribe(mqtt::broker::SocketContext* socketContext) {
        subscribers.erase(socketContext);

        for (auto it = subscribtions.begin(); it != subscribtions.end();) {
            if (it->second.unsubscribe(socketContext)) {
                it = subscribtions.erase(it);
            } else {
                ++it;
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    bool SubscribtionTree::unsubscribe(std::string remainingTopicName, mqtt::broker::SocketContext* socketContext) {
        if (remainingTopicName.empty()) {
            subscribers.erase(socketContext);
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName) && subscribtions[topicName].unsubscribe(remainingTopicName, socketContext)) {
                subscribtions.erase(topicName);
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    void SubscribtionTree::subscribe(std::string remainingTopicName,
                                     const std::string& fullTopicName,
                                     mqtt::broker::SocketContext* socketContext,
                                     uint8_t qoSLevel) {
        if (remainingTopicName.empty()) {
            subscribedTopicName = fullTopicName;
            subscribers[socketContext] = qoSLevel;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            subscribtions[topicName].subscribe(remainingTopicName, fullTopicName, socketContext, qoSLevel);
        }
    }

    void SubscribtionTree::publish(std::string remainingTopicName, const std::string& fullTopicName, const std::string& message) {
        if (remainingTopicName.empty()) {
            for (auto& [subscriber, qoS] : subscribers) {
                LOG(TRACE) << "Send Publich: " << subscribedTopicName << " - " << fullTopicName << " - " << message << " - " << qoS;
                subscriber->sendPublish(fullTopicName, message, 0, qoS, 0);
            }
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName)) {
                subscribtions[topicName].publish(remainingTopicName, fullTopicName, message);
            }
            if (subscribtions.contains("+")) {
                subscribtions["+"].publish(remainingTopicName, fullTopicName, message);
            }
            if (subscribtions.contains("#")) {
                const SubscribtionTree& foundSubscription = subscribtions.find("#")->second;

                for (auto& [subscriber, qoS] : foundSubscription.subscribers) {
                    LOG(TRACE) << "Send Publich: " << subscribedTopicName << " - " << fullTopicName << " - " << message << " - " << qoS;
                    subscriber->sendPublish(fullTopicName, message, 0, qoS, 0);
                }
            }
        }
    }

} // namespace mqtt::broker
