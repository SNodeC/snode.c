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

#ifndef APPS_MQTT_SERVER_BROKER_H
#define APPS_MQTT_SERVER_BROKER_H

#include "apps/mqtt/broker/RetainTree.h"
#include "apps/mqtt/broker/SubscribtionTree.h"

namespace apps::mqtt::broker {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    class Broker {
    private:
        Broker() = default;

    public:
        static Broker& instance();

        ~Broker();

        void subscribe(const std::string& topic, apps::mqtt::broker::SocketContext* socketContext, uint8_t qoSLevel);
        void publish(const std::string& topic, const std::string& message, bool retain);
        void unsubscribe(const std::string& topic, apps::mqtt::broker::SocketContext* socketContext);
        void unsubscribe(apps::mqtt::broker::SocketContext* socketContext);

    private:
        apps::mqtt::broker::SubscribtionTree subscribtionTree;
        apps::mqtt::broker::RetainTree retainTree;
    };

} // namespace apps::mqtt::broker

#endif // APPS_MQTT_SERVER_BROKER_H
