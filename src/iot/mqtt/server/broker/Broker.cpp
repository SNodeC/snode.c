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

#include "iot/mqtt/server/broker/Message.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

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

    uint8_t Broker::subscribe(const std::string& topic, const std::string& clientId, uint8_t suscribedQoS) {
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

    void Broker::publish(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        subscribtionTree.publish(Message(topic, message, qoS), retain);
    }

    void Broker::retain(const std::string& topic, const std::string& message, uint8_t qoS) {
        retainTree.retain(Message(topic, message, qoS));
    }

    void Broker::unsubscribe(const std::string& topic, const std::string& clientId) {
        subscribtionTree.unsubscribe(topic, clientId);
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    bool Broker::hasSession(const std::string& clientId) {
        return sessions.contains(clientId);
    }

    bool Broker::hasActiveSession(const std::string& clientId) {
        return hasSession(clientId) && sessions[clientId].isActive();
    }

    bool Broker::hasRetainedSession(const std::string& clientId) {
        return hasSession(clientId) && sessions[clientId].isRetained();
    }

    void Broker::newSession(const std::string& clientId, SocketContext* socketContext) {
        sessions[clientId] = iot::mqtt::server::broker::Session(socketContext);
    }

    void Broker::renewSession(const std::string& clientId, SocketContext* socketContext) {
        sessions[clientId].renew(socketContext);
        subscribtionTree.publishRetained(clientId);
        sessions[clientId].sendQueuedMessages();
    }

    void Broker::retainSession(const std::string& clientId, SocketContext* socketContext) {
        if (sessions.contains(clientId) && sessions[clientId].isOwner(socketContext)) {
            sessions[clientId].retain();
        }
    }

    void Broker::deleteSession(const std::string& clientId, SocketContext* socketContext) {
        if (sessions.contains(clientId) && sessions[clientId].isOwner(socketContext)) {
            subscribtionTree.unsubscribe(clientId);
            sessions.erase(clientId);
        }
    }

    void Broker::sendPublish(const std::string& clientId, Message& message, bool dup, bool retain, uint8_t clientQoS) {
        if (hasActiveSession(clientId)) {
            LOG(TRACE) << "Send Publish: ClientId = " << clientId;
            LOG(TRACE) << "              TopicName = " << message.getTopic();
            LOG(TRACE) << "              Message = " << message.getMessage();
            LOG(TRACE) << "              QoS = " << static_cast<uint16_t>(std::min(clientQoS, message.getQoS()));

            sessions[clientId].sendPublish(message, dup, retain, clientQoS);
        } else if (hasRetainedSession(clientId)) {
            sessions[clientId].queue(message, clientQoS);
        }
    }

    void Broker::publishRetained(const std::string& topic, const std::string& clientId, uint8_t clientQoS) {
        retainTree.publish(topic, clientId, clientQoS);
    }

} // namespace iot::mqtt::server::broker
