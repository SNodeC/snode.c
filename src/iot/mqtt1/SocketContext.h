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

#ifndef IOT_MQTT_SOCKETCONTEXT_H
#define IOT_MQTT_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
//#include "iot/mqtt/ControlPacketFactory.h"
//#include "iot/mqtt/Topic.h"               // IWYU pragma: export
//#include "iot/mqtt/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Connect.h" // IWYU pragma: export
//#include "iot/mqtt/packets/Disconnect.h"  // IWYU pragma: export
//#include "iot/mqtt/packets/Pingreq.h"     // IWYU pragma: export
//#include "iot/mqtt/packets/Pingresp.h"    // IWYU pragma: export
//#include "iot/mqtt/packets/Puback.h"      // IWYU pragma: export
//#include "iot/mqtt/packets/Pubcomp.h"     // IWYU pragma: export
//#include "iot/mqtt/packets/Publish.h"     // IWYU pragma: export
//#include "iot/mqtt/packets/Pubrec.h"      // IWYU pragma: export
//#include "iot/mqtt/packets/Pubrel.h"      // IWYU pragma: export
//#include "iot/mqtt/packets/Suback.h"      // IWYU pragma: export
//#include "iot/mqtt/packets/Subscribe.h"   // IWYU pragma: export
//#include "iot/mqtt/packets/Unsuback.h"    // IWYU pragma: export
//#include "iot/mqtt/packets/Unsubscribe.h" // IWYU pragma: export

#include "iot/mqtt1/StaticHeader.h"

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt1 {
    class ControlPacket;
}
#include "iot/mqtt1/Topic.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        ~SocketContext();

    private:
        virtual std::size_t onReceiveFromPeer() final;

        void printData(const std::vector<char>& data) const;

        int state = 0;

        StaticHeader staticHeader;

        ControlPacket* currentPacket = nullptr;
    };

} // namespace iot::mqtt1

#endif // IOT_MQTT_SOCKETCONTEXT_H
