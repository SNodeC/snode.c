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

#include "apps/mqtt/broker/TopicTree.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    TopicTree::TopicTree(const std::string& fullTopicName, const std::string& value)
        : fullName(fullTopicName)
        , value(value) {
    }

    void TopicTree::publish(const std::string& fullTopicName, const std::string& value) {
        publish(fullTopicName, fullTopicName, value);
    }

    void TopicTree::traverse() {
        traverse(0);
    }

    void TopicTree::publish(const std::string& fullTopicName, std::string remainingTopicName, const std::string& value) {
        if (remainingTopicName.empty()) {
            this->fullName = fullTopicName;
            this->value = value;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree.contains(topicName)) {
                topicTree.find(topicName)->second.publish(fullTopicName, remainingTopicName, value);
            } else {
                topicTree.insert({topicName, TopicTree(fullTopicName, "")}).first->second.publish(fullTopicName, remainingTopicName, value);
            }
        }
    }

    void TopicTree::traverse(unsigned long level) {
        if (!value.empty()) {
            VLOG(0) << std::string(level * 4, ' ') << "FullName: " << fullName << " = " << value;
        }

        for (auto topicTreeEntry : topicTree) {
            VLOG(0) << std::string(level * 4 + 2, ' ') << "Name: " << topicTreeEntry.first;
            topicTreeEntry.second.traverse(level + 1);
        }
    }

} // namespace apps::mqtt::broker
