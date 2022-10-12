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

#include "iot/mqtt/SocketContext.h"           // IWYU pragma: export
#include "iot/mqtt/client/packets/Connack.h"  // IWYU pragma: export
#include "iot/mqtt/client/packets/Pingresp.h" // IWYU pragma: export
#include "iot/mqtt/client/packets/Puback.h"   // IWYU pragma: export
#include "iot/mqtt/client/packets/Pubcomp.h"  // IWYU pragma: export
#include "iot/mqtt/client/packets/Publish.h"  // IWYU pragma: export
#include "iot/mqtt/client/packets/Pubrec.h"   // IWYU pragma: export
#include "iot/mqtt/client/packets/Pubrel.h"   // IWYU pragma: export
#include "iot/mqtt/client/packets/Suback.h"   // IWYU pragma: export
#include "iot/mqtt/client/packets/Unsuback.h" // IWYU pragma: export
#include "iot/mqtt/packets/Connect.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Disconnect.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Pingreq.h"         // IWYU pragma: export
#include "iot/mqtt/packets/Subscribe.h"       // IWYU pragma: export
#include "iot/mqtt/packets/Unsubscribe.h"     // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

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
        iot::mqtt::ControlPacketDeserializer* onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) final;

        virtual void onConnack(iot::mqtt::packets::Connack& connack) = 0; // Client
        virtual void onPublish(iot::mqtt::packets::Publish& publish) = 0;
        virtual void onPuback(iot::mqtt::packets::Puback& puback) = 0;
        virtual void onPubrec(iot::mqtt::packets::Pubrec& pubrec) = 0;
        virtual void onPubrel(iot::mqtt::packets::Pubrel& pubrel) = 0;
        virtual void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) = 0;
        virtual void onSuback(iot::mqtt::packets::Suback& suback) = 0;       // Client
        virtual void onUnsuback(iot::mqtt::packets::Unsuback& unsuback) = 0; // Client
        virtual void onPingresp(iot::mqtt::packets::Pingresp& pingresp) = 0; // Client

        void __onPublish(iot::mqtt::packets::Publish& publish);

        void _onConnack(iot::mqtt::packets::Connack& connack); // Client
        void _onPublish(iot::mqtt::packets::Publish& publish);
        void _onPuback(iot::mqtt::packets::Puback& puback);
        void _onPubrec(iot::mqtt::packets::Pubrec& pubrec);
        void _onPubrel(iot::mqtt::packets::Pubrel& pubrel);
        void _onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp);
        void _onSuback(iot::mqtt::packets::Suback& suback);       // Client
        void _onUnsuback(iot::mqtt::packets::Unsuback& unsuback); // Client
        void _onPingresp(iot::mqtt::packets::Pingresp& pingresp); // Client

    public:
        void sendConnect(const std::string& clientId);        // Client
        void sendSubscribe(std::list<Topic>& topics);         // Client
        void sendUnsubscribe(std::list<std::string>& topics); // Client
        void sendPingreq();                                   // Client
        void sendDisconnect();                                // Client

        friend class iot::mqtt::packets::deserializer::Connack;
        friend class iot::mqtt::packets::deserializer::Suback;
        friend class iot::mqtt::packets::deserializer::Unsuback;
        friend class iot::mqtt::packets::deserializer::Pingresp;
        friend class iot::mqtt::packets::deserializer::Publish;
        friend class iot::mqtt::packets::deserializer::Pubcomp;
        friend class iot::mqtt::packets::deserializer::Pubrec;
        friend class iot::mqtt::packets::deserializer::Puback;
        friend class iot::mqtt::packets::deserializer::Pubrel;
    };

} // namespace iot::mqtt::client

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
