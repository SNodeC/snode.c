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

#include "iot/mqtt/SocketContext.h" // IWYU pragma: export
/*
#include "iot/mqtt/packets/Disconnect.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Pingreq.h"        // IWYU pragma: export
#include "iot/mqtt/packets/Pingresp.h"       // IWYU pragma: export
#include "iot/mqtt/packets/Puback.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Pubcomp.h"        // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"        // IWYU pragma: export
#include "iot/mqtt/packets/Pubrec.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Pubrel.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Suback.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Subscribe.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Unsuback.h"       // IWYU pragma: export
#include "iot/mqtt/packets/Unsubscribe.h"    // IWYU pragma: export
*/
#include "iot/mqtt/server/packets/Connack.h"   // IWYU pragma: export
#include "iot/mqtt/server/packets/Connect.h"   // IWYU pragma: export
#include "iot/mqtt/server/packets/Suback.h"    // IWYU pragma: export
#include "iot/mqtt/server/packets/Subscribe.h" // IWYU pragma: export

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

namespace iot::mqtt::server {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    private:
        iot::mqtt::ControlPacketReceiver* onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) final;

        virtual void onConnect(iot::mqtt::server::packets::Connect& connect) = 0;
        virtual void onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) = 0; // Server
        /*
                virtual void onUnsubscribe(iot::mqtt::packets::Unsubscribe& unsubscribe) = 0; // Server
                virtual void onPingreq(iot::mqtt::packets::Pingreq& pingreq) = 0;             // Server
                virtual void onDisconnect(iot::mqtt::packets::Disconnect& disconnect) = 0;    // Server
        */

        void _onConnect(iot::mqtt::server::packets::Connect& connect);
        void _onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe); // Server
        /*
                void _onUnsubscribe(iot::mqtt::packets::Unsubscribe& unsubscribe); // Server
                void _onPingreq(iot::mqtt::packets::Pingreq& pingreq);             // Server
                void _onDisconnect(iot::mqtt::packets::Disconnect& disconnect);    // Server
        */

    public:
        void sendConnack(uint8_t returnCode, uint8_t flags);                         // Server
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes); // Server
        /*
                void sendUnsuback(uint16_t packetIdentifier);                                // Server
                void sendPingresp();                                                         // Server
        */

        //        friend class iot::mqtt::packets::Connack;
        friend class iot::mqtt::server::packets::Connect;
        //        friend class iot::mqtt::packets::Publish;
        //        friend class iot::mqtt::packets::Puback;
        //        friend class iot::mqtt::packets::Pubrec;
        //        friend class iot::mqtt::packets::Pubrel;
        //        friend class iot::mqtt::packets::Pubcomp;
        friend class iot::mqtt::server::packets::Subscribe;
        //        friend class iot::mqtt::packets::Suback;
        //        friend class iot::mqtt::packets::Unsubscribe;
        //        friend class iot::mqtt::packets::Unsuback;
        //        friend class iot::mqtt::packets::Pingreq;
        //        friend class iot::mqtt::packets::Pingresp;
        //        friend class iot::mqtt::packets::Disconnect;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
