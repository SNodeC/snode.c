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
        virtual std::size_t onReceiveFromPeer() final;

        virtual void onConnect(const iot::mqtt::packets::Connect& connect) = 0;
        virtual void onConnack(const iot::mqtt::packets::Connack& connack) = 0;
        virtual void onPublish(const iot::mqtt::packets::Publish& publish) = 0;
        virtual void onPuback(const iot::mqtt::packets::Puback& puback) = 0;
        virtual void onPubrec(const iot::mqtt::packets::Pubrec& pubrec) = 0;
        virtual void onPubrel(const iot::mqtt::packets::Pubrel& pubrel) = 0;
        virtual void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) = 0;
        virtual void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) = 0;
        virtual void onSuback(const iot::mqtt::packets::Suback& suback) = 0;
        virtual void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) = 0;
        virtual void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) = 0;
        virtual void onPingreq(const iot::mqtt::packets::Pingreq& pingreq) = 0;
        virtual void onPingresp(const iot::mqtt::packets::Pingresp& pingresp) = 0;
        virtual void onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) = 0;

        void _onConnect(iot::mqtt::packets::Connect& connect);
        void _onConnack(const iot::mqtt::packets::Connack& connack);
        void _onPublish(const iot::mqtt::packets::Publish& publish);
        void _onPuback(const iot::mqtt::packets::Puback& puback);
        void _onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        void _onPubrel(const iot::mqtt::packets::Pubrel& pubrel);
        void _onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);
        void _onSubscribe(const iot::mqtt::packets::Subscribe& subscribe);
        void _onSuback(const iot::mqtt::packets::Suback& suback);
        void _onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe);
        void _onUnsuback(const iot::mqtt::packets::Unsuback& unsuback);
        void _onPingreq(const iot::mqtt::packets::Pingreq& pingreq);
        void _onPingresp(const iot::mqtt::packets::Pingresp& pingresp);
        void _onDisconnect(const iot::mqtt::packets::Disconnect& disconnect);

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
        void send(iot::mqtt::ControlPacket&& controlPacket) const;
        void send(iot::mqtt::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;

        std::string getRandomClientId();

        uint16_t getPacketIdentifier();

        static void printData(const std::vector<char>& data);

        iot::mqtt::StaticHeader staticHeader;
        iot::mqtt::ControlPacket* currentPacket = nullptr;

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
