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

#ifndef IOT_MQTTFAST_SOCKETCONTEXT_H
#define IOT_MQTTFAST_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
#include "iot/mqtt-fast/ControlPacketFactory.h"
#include "iot/mqtt-fast/Topic.h"               // IWYU pragma: export
#include "iot/mqtt-fast/packets/Connack.h"     // IWYU pragma: export
#include "iot/mqtt-fast/packets/Connect.h"     // IWYU pragma: export
#include "iot/mqtt-fast/packets/Disconnect.h"  // IWYU pragma: export
#include "iot/mqtt-fast/packets/Pingreq.h"     // IWYU pragma: export
#include "iot/mqtt-fast/packets/Pingresp.h"    // IWYU pragma: export
#include "iot/mqtt-fast/packets/Puback.h"      // IWYU pragma: export
#include "iot/mqtt-fast/packets/Pubcomp.h"     // IWYU pragma: export
#include "iot/mqtt-fast/packets/Publish.h"     // IWYU pragma: export
#include "iot/mqtt-fast/packets/Pubrec.h"      // IWYU pragma: export
#include "iot/mqtt-fast/packets/Pubrel.h"      // IWYU pragma: export
#include "iot/mqtt-fast/packets/Suback.h"      // IWYU pragma: export
#include "iot/mqtt-fast/packets/Subscribe.h"   // IWYU pragma: export
#include "iot/mqtt-fast/packets/Unsuback.h"    // IWYU pragma: export
#include "iot/mqtt-fast/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt_fast {
    class ControlPacket;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt_fast {

    class SocketContext : public core::socket::SocketContext1 {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        void sendConnect(const std::string& clientId);
        void sendConnack(uint8_t returnCode, uint8_t flags);
        void sendPublish(const std::string& topic, const std::string& message, bool dup = false, uint8_t qoS = 0, bool retain = false);
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
        virtual void onConnect(const iot::mqtt_fast::packets::Connect& connect) = 0;
        virtual void onConnack(const iot::mqtt_fast::packets::Connack& connack) = 0;
        virtual void onPublish(const iot::mqtt_fast::packets::Publish& publish) = 0;
        virtual void onPuback(const iot::mqtt_fast::packets::Puback& puback) = 0;
        virtual void onPubrec(const iot::mqtt_fast::packets::Pubrec& pubrec) = 0;
        virtual void onPubrel(const iot::mqtt_fast::packets::Pubrel& pubrel) = 0;
        virtual void onPubcomp(const iot::mqtt_fast::packets::Pubcomp& pubcomp) = 0;
        virtual void onSubscribe(const iot::mqtt_fast::packets::Subscribe& subscribe) = 0;
        virtual void onSuback(const iot::mqtt_fast::packets::Suback& suback) = 0;
        virtual void onUnsubscribe(const iot::mqtt_fast::packets::Unsubscribe& unsubscribe) = 0;
        virtual void onUnsuback(const iot::mqtt_fast::packets::Unsuback& unsuback) = 0;
        virtual void onPingreq(const iot::mqtt_fast::packets::Pingreq& pingreq) = 0;
        virtual void onPingresp(const iot::mqtt_fast::packets::Pingresp& pingresp) = 0;
        virtual void onDisconnect(const iot::mqtt_fast::packets::Disconnect& disconnect) = 0;

        virtual std::size_t onReceiveFromPeer() final;

        void send(iot::mqtt_fast::ControlPacket&& controlPacket) const;
        void send(iot::mqtt_fast::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;
        void printData(const std::vector<char>& data) const;

        uint16_t getPacketIdentifier() {
            ++_packetIdentifier;

            if (_packetIdentifier == 0) {
                ++_packetIdentifier;
            }

            return _packetIdentifier;
        }

        iot::mqtt_fast::ControlPacketFactory controlPacketFactory;

        uint16_t _packetIdentifier = 0;
    };

} // namespace iot::mqtt_fast

#endif // IOT_MQTTFAST_SOCKETCONTEXT_H
