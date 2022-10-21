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

#include "iot/mqtt/server/broker/Session.h"

#include "iot/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Session::Session(const std::string& clientId, SocketContext* socketContext)
        : clientId(clientId)
        , socketContext(socketContext) {
    }

    void Session::sendPublish(Message& message, bool dup, bool retain, uint8_t clientQoS) {
        LOG(TRACE) << "  Send Publish: ClientId: " << clientId;

        std::stringstream messageData;
        const std::string& mes = message.getMessage();
        std::for_each(mes.begin(), mes.end(), [&messageData](char ch) {
            messageData << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(ch) << " ";
        });

        LOG(TRACE) << "                TopicName: " << message.getTopic();
        LOG(TRACE) << "                Message: " << messageData.str();
        LOG(TRACE) << "                QoS: " << static_cast<uint16_t>(std::min(clientQoS, message.getQoS()));

        if (isActive()) {
            socketContext->sendPublish(
                getPacketIdentifier(), message.getTopic(), message.getMessage(), dup, std::min(message.getQoS(), clientQoS), retain);
        } else {
            if (message.getQoS() == 0) {
                messageQueue.clear();
            }

            Message queuedMessage(message);
            queuedMessage.setQoS(std::min(message.getQoS(), clientQoS));
            messageQueue.push_back(std::move(queuedMessage));
        }
    }

    void Session::publishQueued() {
        LOG(TRACE) << "    send queued messages ...";
        for (iot::mqtt::server::broker::Message& message : messageQueue) {
            sendPublish(message, false, false, message.getQoS());
        }
        LOG(TRACE) << "    ... done";

        messageQueue.clear();
    }

    void Session::renew(iot::mqtt::server::SocketContext* socketContext) {
        this->socketContext = socketContext;
    }

    void Session::retain() {
        this->socketContext = nullptr;
    }

    bool Session::isActive() const {
        return socketContext != nullptr;
    }

    bool Session::isOwnedBy(const SocketContext* socketContext) const {
        return this->socketContext == socketContext;
    }

    uint16_t Session::getPacketIdentifier() {
        ++packetIdentifier;

        if (packetIdentifier == 0) {
            ++packetIdentifier;
        }

        return packetIdentifier;
    }

} // namespace iot::mqtt::server::broker
