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

#ifndef MQTT_SOCKETCONTEXT_H
#define MQTT_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
#include "mqtt/ControlPacketFactory.h"
#include "mqtt/Topic.h"               // IWYU pragma: export
#include "mqtt/packets/Connack.h"     // IWYU pragma: export
#include "mqtt/packets/Connect.h"     // IWYU pragma: export
#include "mqtt/packets/Disconnect.h"  // IWYU pragma: export
#include "mqtt/packets/Pingreq.h"     // IWYU pragma: export
#include "mqtt/packets/Pingresp.h"    // IWYU pragma: export
#include "mqtt/packets/Puback.h"      // IWYU pragma: export
#include "mqtt/packets/Publish.h"     // IWYU pragma: export
#include "mqtt/packets/Suback.h"      // IWYU pragma: export
#include "mqtt/packets/Subscribe.h"   // IWYU pragma: export
#include "mqtt/packets/Unsuback.h"    // IWYU pragma: export
#include "mqtt/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    protected:
        virtual void onControlPackageReceived(std::vector<char>& controlPacket) = 0;

        virtual void onConnect(const mqtt::packets::Connect& connect);
        virtual void onConnack(const mqtt::packets::Connack& connack);
        virtual void onPublish(const mqtt::packets::Publish& publish);
        virtual void onPuback(const mqtt::packets::Puback& puback);
        virtual void onSubscribe(const mqtt::packets::Subscribe& subscribe);
        virtual void onSuback(const mqtt::packets::Suback& suback);
        virtual void onUnsubscribe(const mqtt::packets::Unsubscribe& unsubscribe);
        virtual void onUnsuback(const mqtt::packets::Unsuback& unsuback);
        virtual void onPingreq(const mqtt::packets::Pingreq& pingreq);
        virtual void onPingresp(const mqtt::packets::Pingresp& pingresp);
        virtual void onDisconnect(const mqtt::packets::Disconnect& disconnect);

        //
        //
        //
        // virtual void onPubrec(const mqtt::packets::Pubrec& pubrec);
        // virtual void onPubrel(const mqtt::packets::Pubrel& pubrel);
        // virtual void onPubcomp(const mqtt::packets::Pubcomp& pubcomp);
        //
        //
        //
        //
        //
        //
        //
        // virtual void onAuth(const mqtt::packets::Auth& auth);

        void sendConnect();
        void sendConnack();
        void sendPublish(uint16_t packetIdentifier,
                         const std::string& topic,
                         const std::string& message,
                         bool dup = false,
                         uint8_t qos = 0,
                         bool retain = false);
        void sendPuback(uint16_t packetIdentifier);
        void sendSubscribe(uint16_t packetIdentifier, std::list<std::string>& topics, uint8_t qos = 0);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);
        void sendUnsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics);
        void sendUnsuback(uint16_t packetIdentifier);
        void sendPingreq();
        void sendPingresp();

    private:
        virtual std::size_t onReceiveFromPeer() final;

        void printData(std::vector<char>& data);
        void send(std::vector<char>& data);

        ControlPacketFactory controlPacketFactory;
    };

} // namespace mqtt

#endif // MQTT_SOCKETCONTEXT_H
