/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef IOT_MQTT_TOPIC_H
#define IOT_MQTT_TOPIC_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    class Topic {
    public:
        Topic(const std::string& name, uint8_t qoS);
        Topic(const Topic&) = default;

        Topic& operator=(const Topic&) = default;

        ~Topic();

        const std::string& getName() const;
        uint8_t getQoS() const;

    private:
        std::string name;
        uint8_t qoS;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_TOPIC_H
