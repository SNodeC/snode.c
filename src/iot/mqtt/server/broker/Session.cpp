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

#include "iot/mqtt/server/broker/Session.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Session::Session(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt)
        : clientId(clientId)
        , mqtt(mqtt) {
    }

    void Session::sendPublish(Message& message, uint8_t qoS) {
        LOG(TRACE) << "  Send Publish: ClientId: " << clientId;

        std::stringstream messageData;
        const std::string& mes = message.getMessage();

        unsigned long i = 0;
        for (char ch : mes) {
            if (i != 0 && i % 8 == 0 && i != mes.size()) {
                messageData << std::endl;
                messageData << "                                                                  ";
            }
            ++i;
            messageData << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
                        << " "; // << " | ";
        }

        LOG(TRACE) << "                TopicName: " << message.getTopic();
        LOG(TRACE) << "                Message: " << messageData.str();
        LOG(TRACE) << "                QoS: " << static_cast<uint16_t>(std::min(qoS, message.getQoS()));

        if (isActive()) {
            mqtt->sendPublish(
                message.getTopic(), message.getMessage(), std::min(message.getQoS(), qoS), message.getDup(), message.getRetain());
        } else {
            if (message.getQoS() == 0) {
                messageQueue.clear();
            }

            Message queuedMessage(message);
            queuedMessage.setQoS(std::min(message.getQoS(), qoS));
            messageQueue.push_back(std::move(queuedMessage));
        }
    }

    void Session::publishQueued() {
        LOG(TRACE) << "    send queued messages ...";
        for (iot::mqtt::server::broker::Message& message : messageQueue) {
            sendPublish(message, message.getQoS());
        }
        LOG(TRACE) << "    ... done";

        messageQueue.clear();
    }

    void Session::renew(iot::mqtt::server::Mqtt* mqtt) {
        this->mqtt = mqtt;
    }

    void Session::retain() {
        this->mqtt = nullptr;
    }

    bool Session::isActive() const {
        return mqtt != nullptr;
    }

    bool Session::isOwnedBy(const iot::mqtt::server::Mqtt* mqtt) const {
        return this->mqtt == mqtt;
    }

} // namespace iot::mqtt::server::broker
