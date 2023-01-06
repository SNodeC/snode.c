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

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    std::shared_ptr<Broker> Broker::broker;

    Broker::Broker(uint8_t subscribtionMaxQoS)
        : subscribtionMaxQoS(subscribtionMaxQoS)
        , subscribtionTree(this)
        , retainTree(this) {
    }

    std::shared_ptr<Broker> Broker::instance(uint8_t subscribtionMaxQoS) {
        if (!broker) {
            broker = std::make_shared<Broker>(subscribtionMaxQoS);
        }

        return broker;
    }

    void Broker::publishRetainedMessage(const std::string& clientId, const std::string& topic, uint8_t clientQoS) {
        retainTree.publish(topic, clientId, clientQoS);
    }

    void Broker::retainMessage(const std::string& topic, const std::string& message, uint8_t qoS) {
        retainTree.retain(Message(topic, message, qoS, MQTT_RETAIN_TRUE));
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    void Broker::publish(const std::string& topic, const std::string& message, uint8_t qoS) {
        subscribtionTree.publish(Message(topic, message, qoS, MQTT_RETAIN_FALSE));
    }

    uint8_t Broker::subscribe(const std::string& clientId, const std::string& topic, uint8_t suscribedQoS) {
        uint8_t selectedQoS = std::min(subscribtionMaxQoS, suscribedQoS);
        uint8_t returnCode = 0;

        if (subscribtionTree.subscribe(topic, clientId, selectedQoS)) {
            retainTree.publish(topic, clientId, selectedQoS);

            returnCode = SUBSCRIBTION_SUCCESS | selectedQoS;
        } else {
            returnCode = SUBSCRIBTION_FAILURE;
        }

        return returnCode;
    }

    void Broker::unsubscribe(const std::string& clientId, const std::string& topic) {
        subscribtionTree.unsubscribe(topic, clientId);
    }

    bool Broker::hasSession(const std::string& clientId) {
        return sessions.contains(clientId);
    }

    bool Broker::hasActiveSession(const std::string& clientId) {
        return hasSession(clientId) && sessions[clientId].isActive();
    }

    bool Broker::hasRetainedSession(const std::string& clientId) {
        return hasSession(clientId) && !sessions[clientId].isActive();
    }

    bool Broker::isActiveSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt) {
        return hasSession(clientId) && sessions[clientId].isOwnedBy(mqtt);
    }

    Session* Broker::newSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt) {
        sessions[clientId] = iot::mqtt::server::broker::Session(clientId, mqtt);

        return &sessions[clientId];
    }

    Session* Broker::renewSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt) {
        sessions[clientId].renew(mqtt);
        subscribtionTree.publishRetained(clientId);
        sessions[clientId].publishQueued();

        return &sessions[clientId];
    }

    void Broker::retainSession(const std::string& clientId) {
        sessions[clientId].retain();
    }

    void Broker::deleteSession(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
        sessions.erase(clientId);
    }

    void Broker::sendPublish(const std::string& clientId, Message& message, uint8_t qoS) {
        sessions[clientId].sendPublish(message, qoS);
    }

} // namespace iot::mqtt::server::broker
