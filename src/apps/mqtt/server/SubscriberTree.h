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

#ifndef APPS_MQTT_SERVER_SUBSCRIBERTREE_H
#define APPS_MQTT_SERVER_SUBSCRIBERTREE_H

namespace apps::mqtt::server {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <map>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::server {

    class SubscriberTree {
    public:
        SubscriberTree() = default;

        void subscribe(const std::string& fullName, apps::mqtt::server::SocketContext* socketContext);

        void publish(const std::string& fullTopicName, const std::string& message);

        void unsubscribe(apps::mqtt::server::SocketContext* socketContext);

        void unsubscribe(std::string remainingTopicName, apps::mqtt::server::SocketContext* socketContext);

    private:
        void publish(const std::string& fullTopicName, std::string remainingTopicName, const std::string& message);

        void subscribe(const std::string& fullName, std::string remainingTopicName, apps::mqtt::server::SocketContext* socketContext);

        std::string fullName = "";
        std::list<apps::mqtt::server::SocketContext*> subscribers;

        std::map<std::string, SubscriberTree> subscriberTree;

        static uint16_t packetIdentifier;
    };

} // namespace apps::mqtt::server

#endif // APPS_MQTT_SERVER_SUBSCRIBERTREE_H
