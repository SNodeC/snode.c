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

#ifndef IOT_MQTT_CONTROLPACKETRECEIVER_H
#define IOT_MQTT_CONTROLPACKETRECEIVER_H

namespace iot::mqtt {
    class MqttContext;
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
#include <cstdint> // IWYU pragma: export

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class ControlPacketDeserializer {
    public:
        ControlPacketDeserializer(uint32_t remainingLength, uint8_t flags, uint8_t mustFlags);

        virtual ~ControlPacketDeserializer() = default;

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext);

    private:
        virtual std::size_t deserializeVP(iot::mqtt::MqttContext* mqttContext) = 0;

    public:
        uint32_t getRemainingLength() const;

        bool isComplete() const;
        bool isError() const;

        std::size_t getConsumed() const;

    protected:
        bool complete = false;
        bool error = false;

        uint32_t remainingLength = 0;

        std::size_t consumed = 0;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_CONTROLPACKETRECEIVER_H
