﻿/*
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

#ifndef IOT_MQTT1_CONTROLPACKET_H
#define IOT_MQTT1_CONTROLPACKET_H

namespace iot::mqtt1 {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    class ControlPacket {
    public:
        explicit ControlPacket(uint8_t type, uint8_t reserved = 0);

        virtual ~ControlPacket() = default;

        virtual std::size_t construct(iot::mqtt1::SocketContext* socketContext) = 0;

        uint8_t getType() const;

        uint8_t getReserved() const;

        bool isComplete() const;
        bool isError() const;

    protected:
        bool complete = false;
        bool error = false;

    private:
        uint8_t type = 0;
        uint8_t reserved = 0;
    };

} // namespace iot::mqtt1

#endif // IOT_MQTT_CONTROLPACKET_H
