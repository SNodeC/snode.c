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

#ifndef APPS_MQTT_SERVER_SOCKETCONTEXT_H
#define APPS_MQTT_SERVER_SOCKETCONTEXT_H

#include "iot/mqtt/SocketContext.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace mqtt::broker {
    class Broker;
} // namespace mqtt::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<mqtt::broker::Broker> broker);
        ~SocketContext() override;

    private:
        void onConnect(const iot::mqtt::packets::Connect& connect) override;
        void onConnack(const iot::mqtt::packets::Connack& connack) override;
        void onPublish(const iot::mqtt::packets::Publish& publish) override;
        void onPuback(const iot::mqtt::packets::Puback& puback) override;
        void onPubrec(const iot::mqtt::packets::Pubrec& pubrec) override;
        void onPubrel(const iot::mqtt::packets::Pubrel& pubrel) override;
        void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) override;
        void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) override;
        void onSuback(const iot::mqtt::packets::Suback& suback) override;
        void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) override;
        void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) override;
        void onPingreq(const iot::mqtt::packets::Pingreq& pingreq) override;
        void onPingresp(const iot::mqtt::packets::Pingresp& pingresp) override;
        void onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) override;

        uint64_t subscribtionCount = 0;

        std::shared_ptr<mqtt::broker::Broker> broker;
    };

} // namespace mqtt::broker

#endif // APPS_MQTT_SOCKETCONTEXT_H
