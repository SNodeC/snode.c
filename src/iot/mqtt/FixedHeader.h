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

#ifndef IOT_MQTT_FIXEDHEADER_H
#define IOT_MQTT_FIXEDHEADER_H

#include "iot/mqtt/types/UInt8.h" // IWYU pragma: export
#include "iot/mqtt/types/UIntV.h" // IWYU pragma: export

namespace iot::mqtt {
    class MqttContext; // IWYU pragma: keep
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class FixedHeader {
    public:
        FixedHeader();
        FixedHeader(uint8_t type, uint8_t flags, uint32_t remainingLength = 0);

        ~FixedHeader();

        std::size_t deserialize(iot::mqtt::MqttContext* mqttContext);
        std::vector<char> serialize();

        uint8_t getPacketType() const;
        uint8_t getFlags() const;

        void setRemainingLength(uint32_t remainingLength);
        uint32_t getRemainingLength() const;

        bool isComplete() const;
        bool isError() const;

        void reset();

    private:
        types::UInt8 typeFlags;
        types::UIntV remainingLength;

        bool complete = false;
        bool error = false;

        int state = 0;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_FIXEDHEADER_H
