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

#ifndef MQTT_CONTROLPACKET_H
#define MQTT_CONTROLPACKET_H

namespace mqtt {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    class ControlPacket {
    public:
        ControlPacket(mqtt::SocketContext* socketContext, uint8_t type, uint8_t reserved, std::vector<char>& data);

        uint8_t type();
        uint8_t reserved();

    protected:
        mqtt::SocketContext* socketContext;

        uint8_t _type;
        uint8_t _reserved;
        std::vector<char> data;
    };

} // namespace mqtt

#endif // MQTT_CONTROLPACKET_H
