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

#ifndef IOT_MQTTFAST_CONTROLPACKETFACTORY_H
#define IOT_MQTTFAST_CONTROLPACKETFACTORY_H

#include "iot/mqtt-fast/types/Binary.h"
#include "iot/mqtt-fast/types/Int_1.h"
#include "iot/mqtt-fast/types/Int_V.h"

namespace core::socket {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt_fast {

    class ControlPacketFactory {
    public:
        explicit ControlPacketFactory(core::socket::SocketContext* socketContext);

        std::size_t construct();
        bool isComplete();
        bool isError();
        iot::mqtt_fast::types::Binary& getPacket();
        uint8_t getPacketType();
        uint8_t getPacketFlags();
        uint64_t getRemainingLength();

        void reset();

    private:
        bool completed = false;
        bool error = false;
        int state = 0;

        iot::mqtt_fast::types::Int_1 typeFlags;
        iot::mqtt_fast::types::Int_V remainingLength;
        iot::mqtt_fast::types::Binary data;
    };

} // namespace iot::mqtt_fast

#endif // IOT_MQTTFAST_CONTROLPACKETFACTORY_H
