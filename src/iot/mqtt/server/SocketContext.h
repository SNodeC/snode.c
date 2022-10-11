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

#include "iot/mqtt/SocketContext.h"                    // IWYU pragma: export
#include "iot/mqtt/packets/Connack.h"                  // IWYU pragma: export
#include "iot/mqtt/packets/Pingresp.h"                 // IWYU pragma: export
#include "iot/mqtt/packets/Suback.h"                   // IWYU pragma: export
#include "iot/mqtt/packets/Unsuback.h"                 // IWYU pragma: export
#include "iot/mqtt/packets/deserializer/Connect.h"     // IWYU pragma: export
#include "iot/mqtt/packets/deserializer/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt/packets/deserializer/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt/packets/deserializer/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt/packets/deserializer/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt::server::broker {
    class Broker;
} // namespace iot::mqtt::server::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection,
                               const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker);
        ~SocketContext() override;

    private:
        iot::mqtt::ControlPacketDeserializer* onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) final;

        void initSession();
        void releaseSession();

        virtual void onConnect(iot::mqtt::packets::deserializer::Connect& connect);
        void onPublish(iot::mqtt::packets::Publish& publish) override;
        void onPuback(iot::mqtt::packets::Puback& puback) override;
        void onPubrec(iot::mqtt::packets::Pubrec& pubrec) override;
        void onPubrel(iot::mqtt::packets::Pubrel& pubrel) override;
        void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) override;
        virtual void onSubscribe(iot::mqtt::packets::deserializer::Subscribe& subscribe);
        virtual void onUnsubscribe(iot::mqtt::packets::deserializer::Unsubscribe& unsubscribe);
        virtual void onPingreq(iot::mqtt::packets::deserializer::Pingreq& pingreq);
        virtual void onDisconnect(iot::mqtt::packets::deserializer::Disconnect& disconnect);

        void __onPublish(iot::mqtt::packets::Publish& publish) override;
        void _onConnect(iot::mqtt::packets::deserializer::Connect& connect);
        void _onSubscribe(iot::mqtt::packets::deserializer::Subscribe& subscribe);
        void _onUnsubscribe(iot::mqtt::packets::deserializer::Unsubscribe& unsubscribe);
        void _onPingreq(iot::mqtt::packets::deserializer::Pingreq& pingreq);
        void _onDisconnect(iot::mqtt::packets::deserializer::Disconnect& disconnect);

    public:
        void sendConnack(uint8_t returnCode, uint8_t flags);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);
        void sendUnsuback(uint16_t packetIdentifier);
        void sendPingresp();

    private:
        std::shared_ptr<iot::mqtt::server::broker::Broker> broker;

        // V-Header
        std::string protocol;
        uint8_t level = 0;
        uint8_t connectFlags = 0;
        uint16_t keepAlive = 0;

        // Payload
        std::string clientId;
        std::string willTopic;
        std::string willMessage;
        std::string username;
        std::string password;

        // Derived from flags
        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;

        friend class iot::mqtt::packets::deserializer::Connect;
        friend class iot::mqtt::packets::deserializer::Subscribe;
        friend class iot::mqtt::packets::deserializer::Unsubscribe;
        friend class iot::mqtt::packets::deserializer::Pingreq;
        friend class iot::mqtt::packets::deserializer::Disconnect;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
