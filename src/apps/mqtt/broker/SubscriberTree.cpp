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

    void SubscriberTree::subscribe(const std::string& fullTopicName, apps::mqtt::broker::SocketContext* socketContext) {
        subscribe(fullTopicName, fullTopicName, socketContext);
    }

    void SubscriberTree::publish(const std::string& fullTopicName, const std::string& message) {
        publish(fullTopicName, fullTopicName, message);
    }

    void SubscriberTree::unsubscribe(apps::mqtt::broker::SocketContext* socketContext) {
        subscribers.erase(socketContext);

        for (auto& subscriberTreeEntry : subscriberTree) {
            subscriberTreeEntry.second.unsubscribe(socketContext);
        }
    }

    void SubscriberTree::unsubscribe(std::string remainingTopicName, apps::mqtt::broker::SocketContext* socketContext) {
        if (remainingTopicName.empty()) {
            subscribers.erase(socketContext);
        } else {
            std::string topicName = remainingTopicName.substr(0, fullName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscriberTree.contains(topicName)) {
                subscriberTree.find(topicName)->second.unsubscribe(remainingTopicName, socketContext);
            }
        }
    }

    void SubscriberTree::subscribe(std::string remainingTopicName,
                                   const std::string& fullTopicName,
                                   apps::mqtt::broker::SocketContext* socketContext) {
        if (remainingTopicName.empty()) {
            this->fullName = fullTopicName;
            subscribers.insert(socketContext);
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscriberTree.contains(topicName)) {
                subscriberTree.find(topicName)->second.subscribe(remainingTopicName, fullTopicName, socketContext);
            } else {
                subscriberTree.insert({topicName, SubscriberTree()})
                    .first->second.subscribe(remainingTopicName, fullTopicName, socketContext);
            }
        }
    }

    void SubscriberTree::publish(std::string remainingTopicName, const std::string& fullTopicName, const std::string& message) {
        if (remainingTopicName.empty()) {
            for (apps::mqtt::broker::SocketContext* subscriber : subscribers) {
                LOG(TRACE) << "Send Publish: " << fullName << " - " << fullTopicName << " - " << message;
                subscriber->sendPublish(fullTopicName, message);
            }
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscriberTree.contains(topicName)) {
                subscriberTree.find(topicName)->second.publish(remainingTopicName, fullTopicName, message);
            } else if (subscriberTree.contains("+")) {
                subscriberTree.find("+")->second.publish(remainingTopicName, fullTopicName, message);
            } else if (subscriberTree.contains("#")) {
                const SubscriberTree& foundSubscription = subscriberTree.find("#")->second;

                for (apps::mqtt::broker::SocketContext* subscriber : foundSubscription.subscribers) {
                    LOG(TRACE) << "Send Publish: " << foundSubscription.fullName << " - " << fullTopicName << " - " << message;
                    subscriber->sendPublish(fullTopicName, message);
                }
            }
        }
    }

} // namespace apps::mqtt::broker
