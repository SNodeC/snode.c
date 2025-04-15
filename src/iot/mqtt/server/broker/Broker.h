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

#ifndef IOT_MQTT_SERVER_BROKER_BROKER_H
#define IOT_MQTT_SERVER_BROKER_BROKER_H

#include "iot/mqtt/server/broker/RetainTree.h"
#include "iot/mqtt/server/broker/Session.h" // IWYU pragma: export
#include "iot/mqtt/server/broker/SubscribtionTree.h"

namespace iot::mqtt::server {
    class Mqtt;
} // namespace iot::mqtt::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>

// IWYU pragma: no_include <iterator>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define SUBSCRIBTION_MAX_QOS 0x02

#define SUBSCRIBTION_SUCCESS 0x00
#define SUBSCRIBTION_FAILURE 0x80

namespace iot::mqtt::server::broker {

    class Broker {
    public:
        Broker(uint8_t maxQoS, const std::string& sessionStoreFileName);

        ~Broker();

        static std::shared_ptr<Broker> instance(uint8_t maxQoS, const std::string& sessionStoreFileName);

        void appear(const std::string& clientId, const std::string& topic, uint8_t qoS);
        void unsubscribe(const std::string& clientId);

        void publish(const std::string& originClientId, const std::string& topic, const std::string& message, uint8_t qoS, bool retain);
        uint8_t subscribe(const std::string& clientId, const std::string& topic, uint8_t qoS);
        void unsubscribe(const std::string& clientId, const std::string& topic);

        std::list<std::string> getSubscriptions(const std::string& clientId) const;
        std::map<std::string, std::list<std::pair<std::string, uint8_t>>> getSubscriptionTree() const;

        iot::mqtt::server::broker::RetainTree& getRetainedTree();

        bool hasSession(const std::string& clientId);
        bool hasActiveSession(const std::string& clientId);
        bool hasRetainedSession(const std::string& clientId);

        bool isActiveSession(const std::string& clientId, const Mqtt* mqtt);

        Session* newSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt);
        Session* renewSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt);
        void restartSession(const std::string& clientId);
        void retainSession(const std::string& clientId);
        void deleteSession(const std::string& clientId);

        void sendPublish(const std::string& clientId, Message& message, uint8_t qoS, bool retain);

    private:
        std::string sessionStoreFileName;
        uint8_t maxQoS;

        iot::mqtt::server::broker::SubscribtionTree subscribtionTree;
        iot::mqtt::server::broker::RetainTree retainTree;

        std::map<std::string, iot::mqtt::server::broker::Session> sessionStore;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_BROKER_H
