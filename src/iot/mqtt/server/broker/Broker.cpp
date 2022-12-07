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
        retainTree.retain(Message(topic, message, qoS, MQTT_RETAIN_TRUE, MQTT_DUP_FALSE));
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    void Broker::publish(const std::string& topic, const std::string& message, uint8_t qoS) {
        subscribtionTree.publish(Message(topic, message, qoS, MQTT_RETAIN_FALSE, MQTT_DUP_FALSE));
    }

    void Broker::pubackReceived([[maybe_unused]] const std::string& clintId, [[maybe_unused]] uint16_t packetIdentifier) {
    }

    void Broker::pubrecReceived([[maybe_unused]] const std::string& clintId, [[maybe_unused]] uint16_t packetIdentifier) {
    }

    void Broker::pubrelReceived([[maybe_unused]] const std::string& clintId, [[maybe_unused]] uint16_t packetIdentifier) {
    }

    void Broker::pubcompReceived([[maybe_unused]] const std::string& clintId, [[maybe_unused]] uint16_t packetIdentifier) {
    }

    uint8_t Broker::subscribeReceived(const std::string& clientId, const std::string& topic, uint8_t suscribedQoS) {
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

    void Broker::unsubscribeReceived(const std::string& clientId, const std::string& topic) {
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

    bool Broker::isActiveSession(const std::string& clientId, SocketContext* socketContext) {
        return hasSession(clientId) && sessions[clientId].isOwnedBy(socketContext);
    }

    void Broker::newSession(const std::string& clientId, SocketContext* socketContext) {
        sessions[clientId] = iot::mqtt::server::broker::Session(clientId, socketContext);
    }

    void Broker::renewSession(const std::string& clientId, SocketContext* socketContext) {
        sessions[clientId].renew(socketContext);
        subscribtionTree.publishRetained(clientId);
        sessions[clientId].publishQueued();
    }

    void Broker::retainSession(const std::string& clientId) {
        sessions[clientId].retain();
    }

    void Broker::deleteSession(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
        sessions.erase(clientId);
    }

    void Broker::sendPublish(const std::string& clientId, Message& message, uint8_t qoS, bool dup) {
        sessions[clientId].sendPublish(message, qoS, message.getRetain(), dup);
    }

} // namespace iot::mqtt::server::broker
