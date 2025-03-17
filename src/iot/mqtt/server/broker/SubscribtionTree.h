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

#ifndef IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H
#define IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H

namespace iot::mqtt::server::broker {
    class Broker;
    class Message;
} // namespace iot::mqtt::server::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class SubscribtionTree {
    public:
        explicit SubscribtionTree(iot::mqtt::server::broker::Broker* broker);

        void appear(const std::string& clientId);

        bool subscribe(const std::string& topic, const std::string& clientId, uint8_t qoS);

        void publish(Message&& message);

        void unsubscribe(const std::string& topic, const std::string& clientId);
        void unsubscribe(const std::string& clientId);

        nlohmann::json toJson() const;
        void fromJson(const nlohmann::json& json);

        std::list<std::string> getSubscriptions(const std::string& clientId) const;

        void clear();

    private:
        class TopicLevel {
        public:
            explicit TopicLevel(iot::mqtt::server::broker::Broker* broker, const std::string& topicLevel);

            void appear(const std::string& clientId, const std::string& topic);

            bool subscribe(const std::string& clientId, uint8_t qoS, std::string topic);

            void publish(Message& message, std::string topic);

            bool unsubscribe(const std::string& clientId, std::string topic);
            bool unsubscribe(const std::string& clientId);

            std::list<std::string> getSubscriptions(const std::string& clientId) const;

            TopicLevel& fromJson(const nlohmann::json& json);
            nlohmann::json toJson() const;

            void clear();

        private:
            std::list<std::string> getSubscriptions(const std::string& absoluteTopicLevel, const std::string& clientId) const;

            iot::mqtt::server::broker::Broker* broker;

            std::map<std::string, uint8_t> clientIds;
            std::map<std::string, TopicLevel> topicLevels;

            std::string topicLevel;
        };

        TopicLevel head;
    };

} // namespace iot::mqtt::server::broker

#endif // IOT_MQTT_SERVER_BROKER_SUBSCRIBERTREE_H
