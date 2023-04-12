/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef IOT_MQTT_SERVER_SOCKETCONTEXTFACTORY_H
#define IOT_MQTT_SERVER_SOCKETCONTEXTFACTORY_H

#include "core/socket/stream/SocketContextFactory.h"

namespace core::socket::stream {
    class SocketConnection;
    class SocketContext;
} // namespace core::socket::stream

namespace iot::mqtt::server::broker {
    class Broker;
} // namespace iot::mqtt::server::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server {

    class SocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        SocketContextFactory();

    private:
        virtual core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection,
                                                            std::shared_ptr<iot::mqtt::server::broker::Broker> broker) = 0;

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) final;

        //        std::shared_ptr<iot::mqtt::server::broker::Broker> broker;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXTFACTORY_H
