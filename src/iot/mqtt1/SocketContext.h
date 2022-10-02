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
//#include "iot/mqtt/ControlPacketFactory.h"
//#include "iot/mqtt/Topic.h"               // IWYU pragma: export
#include "iot/mqtt1/ControlPacket.h"
#include "iot/mqtt1/packets/Connack.h" // IWYU pragma: export
#include "iot/mqtt1/packets/Connect.h" // IWYU pragma: export
//#include "iot/mqtt1/packets/Disconnect.h"  // IWYU pragma: export
//#include "iot/mqtt1/packets/Pingreq.h"     // IWYU pragma: export
//#include "iot/mqtt1/packets/Pingresp.h"    // IWYU pragma: export
//#include "iot/mqtt1/packets/Puback.h"      // IWYU pragma: export
//#include "iot/mqtt1/packets/Pubcomp.h"     // IWYU pragma: export
//#include "iot/mqtt1/packets/Publish.h"     // IWYU pragma: export
//#include "iot/mqtt1/packets/Pubrec.h"      // IWYU pragma: export
//#include "iot/mqtt1/packets/Pubrel.h"      // IWYU pragma: export
//#include "iot/mqtt1/packets/Suback.h"      // IWYU pragma: export
//#include "iot/mqtt1/packets/Subscribe.h"   // IWYU pragma: export
//#include "iot/mqtt1/packets/Unsuback.h"    // IWYU pragma: export
//#include "iot/mqtt1/packets/Unsubscribe.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

#include "iot/mqtt1/Topic.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        ~SocketContext();

    private:
        virtual std::size_t onReceiveFromPeer() final;

        //    public:
        void _onConnect(const iot::mqtt1::packets::Connect& connect);
        void _onConnack(const iot::mqtt1::packets::Connack& connack);

        //        void _onPublish(const iot::mqtt::packets::Publish& publish) = 0;
        //        void _onPuback(const iot::mqtt::packets::Puback& puback) = 0;
        //        void _onPubrec(const iot::mqtt::packets::Pubrec& pubrec) = 0;
        //        void _onPubrel(const iot::mqtt::packets::Pubrel& pubrel) = 0;
        //        void _onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) = 0;
        //        void _onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) = 0;
        //        void _onSuback(const iot::mqtt::packets::Suback& suback) = 0;
        //        void _onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) = 0;
        //        void _onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) = 0;
        //        void _onPingreq(const iot::mqtt::packets::Pingreq& pingreq) = 0;
        //        void _onPingresp(const iot::mqtt::packets::Pingresp& pingresp) = 0;
        //        void _onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) = 0;

    private:
        virtual void onConnect(const iot::mqtt1::packets::Connect& connect) = 0;

        void sendConnack(uint8_t returnCode, uint8_t flags);
        //        virtual void onConnack(const iot::mqtt::packets::Connack& connack) = 0;
        //        virtual void onPublish(const iot::mqtt::packets::Publish& publish) = 0;
        //        virtual void onPuback(const iot::mqtt::packets::Puback& puback) = 0;
        //        virtual void onPubrec(const iot::mqtt::packets::Pubrec& pubrec) = 0;
        //        virtual void onPubrel(const iot::mqtt::packets::Pubrel& pubrel) = 0;
        //        virtual void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) = 0;
        //        virtual void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) = 0;
        //        virtual void onSuback(const iot::mqtt::packets::Suback& suback) = 0;
        //        virtual void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) = 0;
        //        virtual void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) = 0;
        //        virtual void onPingreq(const iot::mqtt::packets::Pingreq& pingreq) = 0;
        //        virtual void onPingresp(const iot::mqtt::packets::Pingresp& pingresp) = 0;
        //        virtual void onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) = 0;

        void send(iot::mqtt1::ControlPacket&& controlPacket) const;
        void send(iot::mqtt1::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;

        void printData(const std::vector<char>& data) const;

        ControlPacket controlPacket;

        friend class iot::mqtt1::packets::Connack;
        friend class iot::mqtt1::packets::Connect;
    };

} // namespace iot::mqtt1

#endif // IOT_MQTT_SOCKETCONTEXT_H
