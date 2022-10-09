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
#include "iot/mqtt/StaticHeader.h"
#include "iot/mqtt/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt/packets/Puback.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Pubcomp.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Pubrec.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Pubrel.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

#include "iot/mqtt/Topic.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        virtual ~SocketContext() override;

    private:
        std::size_t onReceiveFromPeer() final;
        //        virtual iot::mqtt::ControlPacket* onReceiveFromPeer(iot::mqtt::StaticHeader& staticHeader) = 0;

        virtual void onConnect(iot::mqtt::packets::Connect& connect) = 0;             // Server
        virtual void onConnack(iot::mqtt::packets::Connack& connack) = 0;             // Client
        virtual void onPublish(iot::mqtt::packets::Publish& publish) = 0;             // Server & Client
        virtual void onPuback(iot::mqtt::packets::Puback& puback) = 0;                // Server & Client
        virtual void onPubrec(iot::mqtt::packets::Pubrec& pubrec) = 0;                // Server & Client
        virtual void onPubrel(iot::mqtt::packets::Pubrel& pubrel) = 0;                // Server & Client
        virtual void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) = 0;             // Server & Client
        virtual void onSubscribe(iot::mqtt::packets::Subscribe& subscribe) = 0;       // Server
        virtual void onSuback(iot::mqtt::packets::Suback& suback) = 0;                // Client
        virtual void onUnsubscribe(iot::mqtt::packets::Unsubscribe& unsubscribe) = 0; // Server
        virtual void onUnsuback(iot::mqtt::packets::Unsuback& unsuback) = 0;          // Client
        virtual void onPingreq(iot::mqtt::packets::Pingreq& pingreq) = 0;             // Server
        virtual void onPingresp(iot::mqtt::packets::Pingresp& pingresp) = 0;          // Client
        virtual void onDisconnect(iot::mqtt::packets::Disconnect& disconnect) = 0;    // Server

        void _onConnect(iot::mqtt::packets::Connect& connect);             // Server
        void _onConnack(iot::mqtt::packets::Connack& connack);             // Client
        void _onPublish(iot::mqtt::packets::Publish& publish);             // Server & Client
        void _onPuback(iot::mqtt::packets::Puback& puback);                // Server & Client
        void _onPubrec(iot::mqtt::packets::Pubrec& pubrec);                // Server & Client
        void _onPubrel(iot::mqtt::packets::Pubrel& pubrel);                // Server & Client
        void _onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp);             // Server & Client
        void _onSubscribe(iot::mqtt::packets::Subscribe& subscribe);       // Server
        void _onSuback(iot::mqtt::packets::Suback& suback);                // Client
        void _onUnsubscribe(iot::mqtt::packets::Unsubscribe& unsubscribe); // Server
        void _onUnsuback(iot::mqtt::packets::Unsuback& unsuback);          // Client
        void _onPingreq(iot::mqtt::packets::Pingreq& pingreq);             // Server
        void _onPingresp(iot::mqtt::packets::Pingresp& pingresp);          // Client
        void _onDisconnect(iot::mqtt::packets::Disconnect& disconnect);    // Server

    public:
        void sendConnect(const std::string& clientId);       // Client
        void sendConnack(uint8_t returnCode, uint8_t flags); // Server
        void sendPublish(const std::string& topic,
                         const std::string& message,
                         bool dup = false,
                         uint8_t qoSLevel = 0,
                         bool retain = false);                                       // Server & Client
        void sendPuback(uint16_t packetIdentifier);                                  // Server & Client
        void sendPubrec(uint16_t packetIdentifier);                                  // Server & Client
        void sendPubrel(uint16_t packetIdentifier);                                  // Server & Client
        void sendPubcomp(uint16_t packetIdentifier);                                 // Server & Client
        void sendSubscribe(std::list<Topic>& topics);                                // Client
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes); // Server
        void sendUnsubscribe(std::list<std::string>& topics);                        // Client
        void sendUnsuback(uint16_t packetIdentifier);                                // Server
        void sendPingreq();                                                          // Client
        void sendPingresp();                                                         // Server
        void sendDisconnect();                                                       // Client

    protected:
        void send(iot::mqtt::ControlPacketSender&& controlPacket) const;
        void send(iot::mqtt::ControlPacketSender& controlPacket) const;
        void send(std::vector<char>&& data) const;

        std::string getRandomClientId();

    private:
        uint16_t getPacketIdentifier();

        static void printData(const std::vector<char>& data);

        iot::mqtt::StaticHeader staticHeader;
        iot::mqtt::ControlPacketReceiver* currentPacket = nullptr;

        uint16_t packetIdentifier = 0;

        int state = 0;

        friend class iot::mqtt::packets::Connack;
        friend class iot::mqtt::packets::Connect;
        friend class iot::mqtt::packets::Publish;
        friend class iot::mqtt::packets::Puback;
        friend class iot::mqtt::packets::Pubrec;
        friend class iot::mqtt::packets::Pubrel;
        friend class iot::mqtt::packets::Pubcomp;
        friend class iot::mqtt::packets::Subscribe;
        friend class iot::mqtt::packets::Suback;
        friend class iot::mqtt::packets::Unsubscribe;
        friend class iot::mqtt::packets::Unsuback;
        friend class iot::mqtt::packets::Pingreq;
        friend class iot::mqtt::packets::Pingresp;
        friend class iot::mqtt::packets::Disconnect;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_SOCKETCONTEXT_H
