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

#ifndef IOT_MQTT_SERVER_BROKER_SESSION_H
#define IOT_MQTT_SERVER_BROKER_SESSION_H

#include "iot/mqtt/Session.h"
#include "iot/mqtt/server/broker/Message.h" // IWYU pragma: export

#include <nlohmann/json_fwd.hpp>

namespace iot::mqtt::server {
    class Mqtt;
} // namespace iot::mqtt::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <deque>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class Session : public iot::mqtt::Session {
    public:
        Session() = default;
        explicit Session(iot::mqtt::server::Mqtt* mqtt);
        Session(const Session&) = default;

        Session& operator=(const Session&) = default;

        void sendPublish(iot::mqtt::server::broker::Message& message, uint8_t qoS);

        void publishQueued();

        Session* renew(iot::mqtt::server::Mqtt* mqtt);

        void retain();

        bool isActive() const;
        bool isOwnedBy(const iot::mqtt::server::Mqtt* mqtt) const;

        nlohmann::json toJson();

    private:
        iot::mqtt::server::Mqtt* mqtt = nullptr;

        std::deque<Message> messageQueue;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_SESSION_H
