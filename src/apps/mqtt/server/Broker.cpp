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

#include "apps/mqtt/server/Broker.h"

#include "apps/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::server {

    Broker::Broker() {
    }

    Broker& Broker::instance() {
        static Broker broker;
        return broker;
    }

    Broker::~Broker() {
    }

    void Broker::subscribe(const std::string& topic, SocketContext* socketContext) {
        topics[topic].push_back(socketContext);
    }

    void Broker::publish(uint16_t packetIdentifier, const std::string& topic, const std::string& message) {
        std::list<apps::mqtt::server::SocketContext*> socketContexts = topics[topic];

        if (!topics[topic].empty()) {
            for (apps::mqtt::server::SocketContext* socketContext : socketContexts) {
                socketContext->sendPublish(packetIdentifier, topic, message);
            }
        } else {
            topics.erase(topic);
        }
    }

    void Broker::unsubscribe(const std::string& topic, SocketContext* socketContext) {
        topics[topic].remove(socketContext);

        if (topics[topic].empty()) {
            topics.erase(topic); // Remove empty topics
        }
    }

    void Broker::unsubscribeAll(SocketContext* socketContext) {
        auto it = topics.begin();

        while (it != topics.end()) {
            it->second.remove(socketContext);

            if (it->second.empty()) { // Remove empty topics
                auto tmpIt = it;
                ++tmpIt;
                topics.erase(it);
                it = tmpIt;
            } else {
                ++it;
            }
        }
    }

} // namespace apps::mqtt::server
