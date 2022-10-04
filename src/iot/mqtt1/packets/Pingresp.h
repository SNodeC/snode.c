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

#ifndef IOT_MQTT_PACKETSNEW_PINGRESP_H
#define IOT_MQTT_PACKETSNEW_PINGRESP_H

#include "iot/mqtt1/ControlPacket.h"

namespace iot::mqtt1 {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define MQTT_PINGRESP 0x0D

namespace iot::mqtt1::packets {

    class Pingresp : public iot::mqtt1::ControlPacket {
    public:
        explicit Pingresp();
        explicit Pingresp(uint32_t remainingLength, uint8_t reserved);

    private:
        std::size_t deserializeVP(SocketContext* socketContext) override;
        std::vector<char> serializeVP() const override;
        void propagateEvent(SocketContext* socketContext) const override;
    };

} // namespace iot::mqtt1::packets

#endif // IOT_MQTT_PACKETSNEW_PINGRESP_H
