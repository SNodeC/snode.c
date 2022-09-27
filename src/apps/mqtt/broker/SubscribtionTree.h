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

namespace apps::mqtt::broker {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    class SubscribtionTree {
    public:
        SubscribtionTree() = default;

        void subscribe(const std::string& fullTopicName, apps::mqtt::broker::SocketContext* socketContext, uint8_t qoSLevel);

        void publish(const std::string& fullTopicName, const std::string& message);

        bool unsubscribe(apps::mqtt::broker::SocketContext* socketContext);

        bool unsubscribe(std::string remainingTopicName, apps::mqtt::broker::SocketContext* socketContext);

    private:
        void subscribe(std::string remainingTopicName,
                       const std::string& fullTopicName,
                       apps::mqtt::broker::SocketContext* socketContext,
                       uint8_t qoSLevel);

        void publish(std::string remainingTopicName, const std::string& fullTopicName, const std::string& message);

        std::map<apps::mqtt::broker::SocketContext*, uint8_t> subscribers;
        std::map<std::string, SubscribtionTree> subscribtions;

        std::string subscribedTopicName = "";
    };

} // namespace apps::mqtt::broker

#endif // APPS_MQTT_SERVER_SUBSCRIBERTREE_H
