/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "iot/mqtt/server/broker/Session.h"

#include "iot/mqtt/server/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Session::Session(iot::mqtt::server::Mqtt* mqtt)
        : mqtt(mqtt) {
    }

    void Session::sendPublish(Message& message, uint8_t qoS, bool retain) {
        LOG(INFO) << "MQTT Broker:   TopicName: " << message.getTopic();
        LOG(INFO) << "MQTT Broker:   Message:\n" << iot::mqtt::Mqtt::toHexString(message.getMessage());
        LOG(DEBUG) << "MQTT Broker:   QoS: " << static_cast<uint16_t>(std::min(qoS, message.getQoS()));

        if (isActive()) {
            LOG(DEBUG) << "MQTT Broker:   ClientId: " << mqtt->getClientId();
            LOG(DEBUG) << "MQTT Broker:   OriginClientId: " << message.getOriginClientId();

            if ((mqtt->getReflect() || mqtt->getClientId() != message.getOriginClientId())) {
                mqtt->sendPublish(message.getTopic(),
                                  message.getMessage(),
                                  std::min(message.getQoS(), qoS),
                                  !mqtt->getReflect() ? message.getOriginRetain() || retain : retain);
            } else {
                LOG(INFO) << "MQTT Broker:     Suppress reflection to origin to avoid message looping";
            }
        } else {
            if (message.getQoS() == 0) {
                messageQueue.clear();
            }

            message.setQoS(std::min(message.getQoS(), qoS));
            messageQueue.emplace_back(message);
        }
    }

    void Session::publishQueued() {
        LOG(INFO) << "MQTT Broker:     send queued messages ...";
        for (iot::mqtt::server::broker::Message& message : messageQueue) {
            sendPublish(message, message.getQoS(), false);
        }
        LOG(INFO) << "MQTT Broker:     ... done";

        messageQueue.clear();
    }

    Session* Session::renew(iot::mqtt::server::Mqtt* mqtt) {
        this->mqtt = mqtt;

        return this;
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

    nlohmann::json Session::toJson() const {
        nlohmann::json json = iot::mqtt::Session::toJson();

        std::transform(messageQueue.begin(), messageQueue.end(), std::back_inserter(json["message_queue"]), [](const Message& message) {
            return message.toJson();
        });

        return json;
    }

    void Session::fromJson(const nlohmann::json& json) {
        std::transform(json["message_queue"].begin(),
                       json["message_queue"].end(),
                       std::back_inserter(messageQueue),
                       [](const nlohmann::json& jsonMessage) {
                           return Message().fromJson(jsonMessage);
                       });

        iot::mqtt::Session::fromJson(json);
    }

} // namespace iot::mqtt::server::broker
