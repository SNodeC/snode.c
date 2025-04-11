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

#ifndef IOT_MQTT_SERVER_BROKER_TOPICTREE_H
#define IOT_MQTT_SERVER_BROKER_TOPICTREE_H

#include "iot/mqtt/server/broker/Message.h"

namespace iot::mqtt::server::broker {
    class Broker;
} // namespace iot::mqtt::server::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    class RetainTree {
    public:
        explicit RetainTree(iot::mqtt::server::broker::Broker* broker);

        void retain(Message&& message);
        void appear(const std::string& clientId, const std::string& topic, uint8_t qoS);

        nlohmann::json toJson() const;
        void fromJson(const nlohmann::json& json);

        std::list<std::pair<std::string, std::string>> getRetainedTree() const;

        void clear();

    private:
        class TopicLevel {
        public:
            explicit TopicLevel(iot::mqtt::server::broker::Broker* broker);

            void retain(const Message& message, std::string topic);
            bool release(std::string topic);

            void appear(const std::string& clientId, std::string topic, uint8_t qoS);

            std::list<std::pair<std::string, std::string>> getRetainTree() const;

            TopicLevel& fromJson(const nlohmann::json& json);
            nlohmann::json toJson() const;

            void clear();

        private:
            void appear(const std::string& clientId, uint8_t clientQoS);

            std::list<std::pair<std::string, std::string>> getRetainTree(const std::string& absoluteTopicLevel) const;

            Message message;

            std::map<std::string, TopicLevel> subTopicLevels;

            iot::mqtt::server::broker::Broker* broker = nullptr;
        };

        TopicLevel head;
    };

} // namespace iot::mqtt::server::broker

#endif // APPS_MQTT_SERVER_TOPICTREE_H
