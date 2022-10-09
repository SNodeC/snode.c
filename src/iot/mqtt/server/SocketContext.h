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

#ifndef IOT_MQTT_SERVER_SOCKETCONTEXT_H
#define IOT_MQTT_SERVER_SOCKETCONTEXT_H

#include "iot/mqtt/SocketContext.h"              // IWYU pragma: export
#include "iot/mqtt/server/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt/server/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt/server/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt/server/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt/server/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt/server/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt/server/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt/server/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt/server/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    private:
        iot::mqtt::ControlPacketReceiver* deserialize(iot::mqtt::StaticHeader& staticHeader) final;

        virtual void onConnect(iot::mqtt::server::packets::Connect& connect) = 0;
        virtual void onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) = 0;
        virtual void onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) = 0;
        virtual void onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) = 0;
        virtual void onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) = 0;

        void _onConnect(iot::mqtt::server::packets::Connect& connect);
        void _onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe);
        void _onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe);
        void _onPingreq(iot::mqtt::server::packets::Pingreq& pingreq);
        void _onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect);

    public:
        void sendConnack(uint8_t returnCode, uint8_t flags);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);
        void sendUnsuback(uint16_t packetIdentifier);
        void sendPingresp();

        friend class iot::mqtt::server::packets::Connect;
        friend class iot::mqtt::server::packets::Subscribe;
        friend class iot::mqtt::server::packets::Unsubscribe;
        friend class iot::mqtt::server::packets::Pingreq;
        friend class iot::mqtt::server::packets::Disconnect;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
