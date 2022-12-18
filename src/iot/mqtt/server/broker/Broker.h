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

#ifndef IOT_MQTT_SERVER_BROKER_BROKER_H
#define IOT_MQTT_SERVER_BROKER_BROKER_H

#include "iot/mqtt/server/broker/RetainTree.h"
#include "iot/mqtt/server/broker/Session.h"
#include "iot/mqtt/server/broker/SubscribtionTree.h"

namespace iot::mqtt::server {
    class Mqtt;

} // namespace iot::mqtt::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define SUBSCRIBTION_MAX_QOS 0x02

#define SUBSCRIBTION_SUCCESS 0x00
#define SUBSCRIBTION_FAILURE 0x80

namespace iot::mqtt::server::broker {

    class Broker {
    public:
        explicit Broker(uint8_t subscribtionMaxQoS);

        static std::shared_ptr<Broker> instance(uint8_t subscribtionMaxQoS);

        void publishRetainedMessage(const std::string& clientId, const std::string& topic, uint8_t clientQoS);
        void retainMessage(const std::string& topic, const std::string& message, uint8_t qoS);
        void unsubscribe(const std::string& clientId);

        void publish(const std::string& topic, const std::string& message, uint8_t qoS);
        void pubackReceived(const std::string& clintId, uint16_t packetIdentifier);
        void pubrecReceived(const std::string& clintId, uint16_t packetIdentifier);
        void pubrelReceived(const std::string& clintId, uint16_t packetIdentifier);
        void pubcompReceived(const std::string& clintId, uint16_t packetIdentifier);
        uint8_t subscribeReceived(const std::string& clientId, const std::string& topic, uint8_t suscribedQoS);
        void unsubscribeReceived(const std::string& clientId, const std::string& topic);

        bool hasSession(const std::string& clientId);
        bool hasActiveSession(const std::string& clientId);
        bool hasRetainedSession(const std::string& clientId);

        bool isActiveSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt);

        void newSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt);
        void renewSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt);
        void retainSession(const std::string& clientId);
        void deleteSession(const std::string& clinetId);

        void sendPublish(const std::string& clientId, Message& message, uint8_t qoS);

    private:
        uint8_t subscribtionMaxQoS;

        iot::mqtt::server::broker::SubscribtionTree subscribtionTree;
        iot::mqtt::server::broker::RetainTree retainTree;

        std::map<std::string, iot::mqtt::server::broker::Session> sessions;

        static std::shared_ptr<Broker> broker;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER__BROKER_H
