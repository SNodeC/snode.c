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

#ifndef IOT_MQTT1_SOCKETCONTEXT_H
#define IOT_MQTT1_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
#include "iot/mqtt1/ControlPacketFactory.h"
#include "iot/mqtt1/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt1/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt1/packets/Puback.h"      // IWYU pragma: export
#include "iot/mqtt1/packets/Pubcomp.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Publish.h"     // IWYU pragma: export
#include "iot/mqtt1/packets/Pubrec.h"      // IWYU pragma: export
#include "iot/mqtt1/packets/Pubrel.h"      // IWYU pragma: export
#include "iot/mqtt1/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt1/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt1/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt1/packets/Unsubscribe.h" // IWYU pragma: export

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

        virtual ~SocketContext();

    private:
        virtual std::size_t onReceiveFromPeer() final;

        virtual void onConnect(const iot::mqtt1::packets::Connect& connect) = 0;
        virtual void onConnack(const iot::mqtt1::packets::Connack& connack) = 0;
        virtual void onPublish(const iot::mqtt1::packets::Publish& publish) = 0;
        virtual void onPuback(const iot::mqtt1::packets::Puback& puback) = 0;
        virtual void onPubrec(const iot::mqtt1::packets::Pubrec& pubrec) = 0;
        virtual void onPubrel(const iot::mqtt1::packets::Pubrel& pubrel) = 0;
        virtual void onPubcomp(const iot::mqtt1::packets::Pubcomp& pubcomp) = 0;
        virtual void onSubscribe(const iot::mqtt1::packets::Subscribe& subscribe) = 0;
        virtual void onSuback(const iot::mqtt1::packets::Suback& suback) = 0;
        virtual void onUnsubscribe(const iot::mqtt1::packets::Unsubscribe& unsubscribe) = 0;
        virtual void onUnsuback(const iot::mqtt1::packets::Unsuback& unsuback) = 0;
        virtual void onPingreq(const iot::mqtt1::packets::Pingreq& pingreq) = 0;
        virtual void onPingresp(const iot::mqtt1::packets::Pingresp& pingresp) = 0;
        virtual void onDisconnect(const iot::mqtt1::packets::Disconnect& disconnect) = 0;

        void _onConnect(const iot::mqtt1::packets::Connect& connect);
        void _onConnack(const iot::mqtt1::packets::Connack& connack);
        void _onPublish(const iot::mqtt1::packets::Publish& publish);
        void _onPuback(const iot::mqtt1::packets::Puback& puback);
        void _onPubrec(const iot::mqtt1::packets::Pubrec& pubrec);
        void _onPubrel(const iot::mqtt1::packets::Pubrel& pubrel);
        void _onPubcomp(const iot::mqtt1::packets::Pubcomp& pubcomp);
        void _onSubscribe(const iot::mqtt1::packets::Subscribe& subscribe);
        void _onSuback(const iot::mqtt1::packets::Suback& suback);
        void _onUnsubscribe(const iot::mqtt1::packets::Unsubscribe& unsubscribe);
        void _onUnsuback(const iot::mqtt1::packets::Unsuback& unsuback);
        void _onPingreq(const iot::mqtt1::packets::Pingreq& pingreq);
        void _onPingresp(const iot::mqtt1::packets::Pingresp& pingresp);
        void _onDisconnect(const iot::mqtt1::packets::Disconnect& disconnect);

    public:
        void sendConnect(const std::string& clientId);
        void sendConnack(uint8_t returnCode, uint8_t flags);
        void sendPublish(const std::string& topic, const std::string& message, bool dup = false, uint8_t qoSLevel = 0, bool retain = false);
        void sendPuback(uint16_t packetIdentifier);
        void sendPubrec(uint16_t packetIdentifier);
        void sendPubrel(uint16_t packetIdentifier);
        void sendPubcomp(uint16_t packetIdentifier);
        void sendSubscribe(std::list<Topic>& topics);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);
        void sendUnsubscribe(std::list<std::string>& topics);
        void sendUnsuback(uint16_t packetIdentifier);
        void sendPingreq();
        void sendPingresp();
        void sendDisconnect();

    private:
        void send(iot::mqtt1::ControlPacket&& controlPacket) const;
        void send(iot::mqtt1::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;

        static void printData(const std::vector<char>& data);

        uint16_t getPacketIdentifier();

        uint16_t packetIdentifier = 0;

        ControlPacketFactory controlPacketFactory;

        iot::mqtt1::ControlPacket* currentPacket = nullptr;

        bool error = false;
        bool complete = false;
        int state = 0;

        friend class iot::mqtt1::packets::Connack;
        friend class iot::mqtt1::packets::Connect;
        friend class iot::mqtt1::packets::Publish;
        friend class iot::mqtt1::packets::Puback;
        friend class iot::mqtt1::packets::Pubrec;
        friend class iot::mqtt1::packets::Pubrel;
        friend class iot::mqtt1::packets::Pubcomp;
        friend class iot::mqtt1::packets::Subscribe;
        friend class iot::mqtt1::packets::Suback;
        friend class iot::mqtt1::packets::Unsubscribe;
        friend class iot::mqtt1::packets::Unsuback;
        friend class iot::mqtt1::packets::Pingreq;
        friend class iot::mqtt1::packets::Pingresp;
        friend class iot::mqtt1::packets::Disconnect;
    };

} // namespace iot::mqtt1

#endif // IOT_MQTT1_SOCKETCONTEXT_H
