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

#include "broker/RetainTree.h"

#include "broker/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    void RetainTree::retain(const std::string& fullTopicName, const std::string& message) {
        retain(fullTopicName, fullTopicName, message);
    }

    bool RetainTree::retain(const std::string& fullTopicName, std::string remainingTopicName, const std::string& message) {
        if (remainingTopicName.empty()) {
            LOG(TRACE) << "Retaining: " << fullTopicName << " - " << message;
            this->fullTopicName = fullTopicName;
            this->message = message;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree[topicName].retain(fullTopicName, remainingTopicName, message)) {
                topicTree.erase(topicName);
            }
        }

        return this->message.empty() && topicTree.empty();
    }

    void RetainTree::publish(std::string remainingTopicName, SocketContext* socketContext, uint8_t qoSLevel) {
        if (remainingTopicName.empty() && !message.empty()) {
            LOG(TRACE) << "Send Publish (retained): " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
            socketContext->sendPublish(fullTopicName, message, 0, qoSLevel, true);
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree.contains(topicName)) {
                topicTree[topicName].publish(remainingTopicName, socketContext, qoSLevel);
            } else if (topicName == "+") {
                for (auto& topicTreeEntry : topicTree) {
                    topicTreeEntry.second.publish(remainingTopicName, socketContext, qoSLevel);
                }
            } else if (topicName == "#") {
                publish(socketContext, qoSLevel);
            }
        }
    }

    void RetainTree::publish(SocketContext* socketContext, uint8_t qoSLevel) {
        LOG(TRACE) << "Send Publish (retained): " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
        if (!message.empty()) {
            socketContext->sendPublish(fullTopicName, message, 0, qoSLevel, true);
        }

        for (auto& [topicName, topicTree] : topicTree) {
            topicTree.publish(socketContext, qoSLevel);
        }
    }

} // namespace mqtt::broker
