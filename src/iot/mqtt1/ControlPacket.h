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

#ifndef IOT_MQTT1_CONTROLPACKET_H
#define IOT_MQTT1_CONTROLPACKET_H

namespace iot::mqtt1 {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    class ControlPacket {
    public:
        explicit ControlPacket(uint8_t type, uint8_t reserved, uint32_t remainingLength);

        virtual ~ControlPacket();

        std::size_t deserialize(iot::mqtt1::SocketContext* socketContext);
        std::vector<char> serialize() const;
        virtual void propagateEvent(SocketContext* socketContext) const = 0;

    private:
        virtual std::size_t deserializeVP(iot::mqtt1::SocketContext* socketContext) = 0;
        virtual std::vector<char> serializeVP() const = 0;

    public:
        uint8_t getType() const;
        uint8_t getFlags() const;
        uint32_t getRemainingLength() const;

        bool isComplete() const;
        bool isError() const;

        std::size_t getConsumed() const;

    protected:
        bool complete = false;
        bool error = false;

    private:
        ControlPacket* currentPacket = nullptr;

        uint8_t type = 0;
        uint8_t flags = 0;
        uint32_t remainingLength = 0;

        std::size_t consumed = 0;

        int state = 0;
    };

} // namespace iot::mqtt1

#endif // IOT_MQTT_CONTROLPACKET_H
