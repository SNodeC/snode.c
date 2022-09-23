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
#include "iot/mqtt/ControlPacketFactory.h"
#include "iot/mqtt/Topic.h"               // IWYU pragma: export
#include "iot/mqtt/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt/packets/Puback.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    class ControlPacket;
}

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

        void sendConnect();
        void sendConnack();
        void sendPublish(uint16_t packetIdentifier,
                         const std::string& topic,
                         const std::string& message,
                         bool dup = false,
                         uint8_t qoSLevel = 0,
                         bool retain = false);
        void sendPuback(uint16_t packetIdentifier);
        void sendSubscribe(uint16_t packetIdentifier, std::list<Topic>& topics);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);
        void sendUnsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics);
        void sendUnsuback(uint16_t packetIdentifier);
        void sendPingreq();
        void sendPingresp();
        void sendDisconnect();

    protected:
        virtual void onConnect(const iot::mqtt::packets::Connect& connect) = 0;
        virtual void onConnack(const iot::mqtt::packets::Connack& connack) = 0;
        virtual void onPublish(const iot::mqtt::packets::Publish& publish) = 0;
        virtual void onPuback(const iot::mqtt::packets::Puback& puback) = 0;
        virtual void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) = 0;
        virtual void onSuback(const iot::mqtt::packets::Suback& suback) = 0;
        virtual void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) = 0;
        virtual void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) = 0;
        virtual void onPingreq(const iot::mqtt::packets::Pingreq& pingreq) = 0;
        virtual void onPingresp(const iot::mqtt::packets::Pingresp& pingresp) = 0;
        virtual void onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) = 0;

        // virtual void onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        // virtual void onPubrel(const iot::mqtt::packets::Pubrel& pubrel);
        // virtual void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);
        // virtual void onAuth(const iot::mqtt::packets::Auth& auth);

    private:
        virtual std::size_t onReceiveFromPeer() final;

        void send(iot::mqtt::ControlPacket&& controlPacket) const;
        void send(iot::mqtt::ControlPacket& controlPacket) const;
        void send(const std::vector<char>& data) const;
        void printData(const std::vector<char>& data) const;

        iot::mqtt::ControlPacketFactory controlPacketFactory;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_SOCKETCONTEXT_H
