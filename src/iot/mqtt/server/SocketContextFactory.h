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

#include "core/socket/SocketContext.h"
#include "core/socket/SocketContextFactory.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace iot::mqtt::server {
    class SocketContext;

    namespace broker {
        class Broker;
    }

} // namespace iot::mqtt::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    template <typename SocketContextT = iot::mqtt::server::SocketContext>
    class SocketContextFactory : public core::socket::SocketContextFactory {
        using SocketContext = SocketContextT;

    public:
        SocketContextFactory();

    private:
        virtual core::socket::SocketContext* create(core::socket::SocketConnection* socketConnection,
                                                    std::shared_ptr<iot::mqtt::server::broker::Broker>& broker) = 0;

        core::socket::SocketContext* create(core::socket::SocketConnection* socketConnection) final;

        std::shared_ptr<iot::mqtt::server::broker::Broker> broker;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXTFACTORY_H
