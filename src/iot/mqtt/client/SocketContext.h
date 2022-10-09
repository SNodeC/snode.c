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
#include "iot/mqtt/client/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt/client/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt/client/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt/client/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt/client/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt/client/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt/client/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt/client/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt/client/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    class StaticHeader;
}

#include "iot/mqtt/Topic.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    private:
        iot::mqtt::ControlPacketReceiver* onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) final;

        virtual void onConnack(iot::mqtt::client::packets::Connack& connack) = 0;    // Client
        virtual void onSuback(iot::mqtt::client::packets::Suback& suback) = 0;       // Client
        virtual void onUnsuback(iot::mqtt::client::packets::Unsuback& unsuback) = 0; // Client
        virtual void onPingresp(iot::mqtt::client::packets::Pingresp& pingresp) = 0; // Client

        void _onConnack(iot::mqtt::client::packets::Connack& connack);    // Client
        void _onSuback(iot::mqtt::client::packets::Suback& suback);       // Client
        void _onUnsuback(iot::mqtt::client::packets::Unsuback& unsuback); // Client
        void _onPingresp(iot::mqtt::client::packets::Pingresp& pingresp); // Client

    public:
        void sendConnect(const std::string& clientId);        // Client
        void sendSubscribe(std::list<Topic>& topics);         // Client
        void sendUnsubscribe(std::list<std::string>& topics); // Client
        void sendPingreq();                                   // Client
        void sendDisconnect();                                // Client

        friend class iot::mqtt::client::packets::Connack;
        friend class iot::mqtt::client::packets::Suback;
        friend class iot::mqtt::client::packets::Unsuback;
        friend class iot::mqtt::client::packets::Pingresp;
    };

} // namespace iot::mqtt::client

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
