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

#include "apps/mqtt/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::broker {

    Broker::Broker()
        : topicTree(apps::mqtt::broker::TopicTree("", "")) {
    }

    Broker& Broker::instance() {
        static Broker broker;
        return broker;
    }

    Broker::~Broker() {
    }

    void Broker::subscribe(const std::string& topic, apps::mqtt::broker::SocketContext* socketContext) {
        subscriberTree.subscribe(topic, socketContext);
    }

    void Broker::publish([[maybe_unused]] uint16_t packetIdentifier, const std::string& topic, const std::string& message) {
        subscriberTree.publish(topic, message);
    }

    void Broker::unsubscribe(const std::string& topic, apps::mqtt::broker::SocketContext* socketContext) {
        subscriberTree.unsubscribe(topic, socketContext);
    }

    void Broker::unsubscribeFromAll(apps::mqtt::broker::SocketContext* socketContext) {
        subscriberTree.unsubscribe(socketContext);
    }

} // namespace apps::mqtt::broker
