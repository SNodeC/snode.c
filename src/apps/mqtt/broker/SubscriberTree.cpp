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

#include "apps/mqtt/broker/SubscriberTree.h"

#include "apps/mqtt/broker/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    void SubscriberTree::subscribe(const std::string& fullTopicName, apps::mqtt::broker::SocketContext* socketContext, uint8_t qoSLevel) {
        subscribe(fullTopicName, fullTopicName, socketContext, qoSLevel);
    }

    void SubscriberTree::publish(const std::string& fullTopicName, const std::string& message) {
        publish(fullTopicName, fullTopicName, message);
    }

    bool SubscriberTree::unsubscribe(apps::mqtt::broker::SocketContext* socketContext) {
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

    bool SubscriberTree::unsubscribe(std::string remainingTopicName, apps::mqtt::broker::SocketContext* socketContext) {
        if (remainingTopicName.empty()) {
            subscribers.erase(socketContext);
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName) && subscribtions.find(topicName)->second.unsubscribe(remainingTopicName, socketContext)) {
                subscribtions.erase(topicName);
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    void SubscriberTree::subscribe(std::string remainingTopicName,
                                   const std::string& fullTopicName,
                                   apps::mqtt::broker::SocketContext* socketContext,
                                   uint8_t qoSLevel) {
        if (remainingTopicName.empty()) {
            this->fullName = fullTopicName;
            subscribers[socketContext] = qoSLevel;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName)) {
                subscribtions.find(topicName)->second.subscribe(remainingTopicName, fullTopicName, socketContext, qoSLevel);
            } else {
                subscribtions.insert({topicName, SubscriberTree()})
                    .first->second.subscribe(remainingTopicName, fullTopicName, socketContext, qoSLevel);
            }
        }
    }

    void SubscriberTree::publish(std::string remainingTopicName, const std::string& fullTopicName, const std::string& message) {
        if (remainingTopicName.empty()) {
            for (auto& sub : subscribers) {
                LOG(TRACE) << "Send Publich: " << fullName << " - " << fullTopicName << " - " << message << " - " << sub.second;
                sub.first->sendPublish(fullTopicName, message, 0, sub.second, 0);
            }
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName)) {
                subscribtions.find(topicName)->second.publish(remainingTopicName, fullTopicName, message);
            }
            if (subscribtions.contains("+")) {
                subscribtions.find("+")->second.publish(remainingTopicName, fullTopicName, message);
            }
            if (subscribtions.contains("#")) {
                const SubscriberTree& foundSubscription = subscribtions.find("#")->second;

                for (auto& subscriber : foundSubscription.subscribers) {
                    LOG(TRACE) << "Send Publich: " << fullName << " - " << fullTopicName << " - " << message << " - " << subscriber.second;
                    subscriber.first->sendPublish(fullTopicName, message, 0, subscriber.second, 0);
                }
            }
        }
    }

} // namespace apps::mqtt::broker
