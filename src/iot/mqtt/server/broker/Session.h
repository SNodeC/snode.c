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

#ifndef IOT_MQTT_SERVER_BROKER_SESSION_H
#define IOT_MQTT_SERVER_BROKER_SESSION_H

#include "iot/mqtt/server/broker/Message.h"

namespace iot::mqtt::server {
    class SocketContext;
} // namespace iot::mqtt::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <deque>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class Session {
    public:
        Session() = default;
        explicit Session(iot::mqtt::server::SocketContext* socketContext);
        Session(const Session&) = default;

        Session& operator=(const Session&) = default;

        ~Session();

        void sendPublish(iot::mqtt::server::broker::Message& message, bool dup, bool retain, uint8_t clientQoS);

        void renew(iot::mqtt::server::SocketContext* socketContext);

        void retain();

        bool isActive() const;
        bool isOwnedBy(const iot::mqtt::server::SocketContext* socketContext) const;

        iot::mqtt::server::SocketContext* getSocketContext() const;

    private:
        uint16_t getPacketIdentifier();

        iot::mqtt::server::SocketContext* socketContext = nullptr;
        std::deque<Message> messageQueue;

        uint16_t packetIdentifier = 0;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_SESSION_H
