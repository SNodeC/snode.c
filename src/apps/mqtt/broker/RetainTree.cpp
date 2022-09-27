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

#include "apps/mqtt/broker/RetainTree.h"

#include "apps/mqtt/broker/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    void RetainTree::retain(const std::string& fullTopicName, const std::string& value) {
        retain(fullTopicName, fullTopicName, value);
    }

    bool RetainTree::retain(const std::string& fullTopicName, std::string remainingTopicName, const std::string& value) {
        if (remainingTopicName.empty()) {
            LOG(TRACE) << "Retaining: " << fullTopicName << " - " << value;
            this->fullName = fullTopicName;
            this->value = value;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree[topicName].retain(fullTopicName, remainingTopicName, value)) {
                topicTree.erase(topicName);
            }
        }

        return this->value.empty() && topicTree.empty();
    }

    void RetainTree::publish(std::string remainingTopicName, SocketContext* socketContext, uint8_t qoSLevel) {
        if (remainingTopicName.empty()) {
            LOG(TRACE) << "Send Publish (retained): " << fullName << " - " << value << " - " << static_cast<uint16_t>(qoSLevel);
            socketContext->sendPublish(fullName, value, 0, qoSLevel, true);
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
        LOG(TRACE) << "Send Publish (retained): " << fullName << " - " << value << " - " << static_cast<uint16_t>(qoSLevel);
        if (!value.empty()) {
            socketContext->sendPublish(fullName, value, 0, qoSLevel, true);
        }

        for (auto& topicTreeEntry : topicTree) {
            topicTreeEntry.second.publish(socketContext, qoSLevel);
        }
    }

} // namespace apps::mqtt::broker
