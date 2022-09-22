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

#include "apps/mqtt/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqtt::server {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : ::mqtt::SocketContext(socketConnection) {
    }

    void SocketContext::onControlPackageReceived([[maybe_unused]] std::vector<char>& controlPacket) {
        VLOG(0) << "ControlPacket received";
    }

    void SocketContext::onConnect(const ::mqtt::packets::Connect& connect) {
        VLOG(0) << "CONNECT";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(connect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connect.getReserved());
        VLOG(0) << "RemainingLength: " << connect.getRemainingLength();
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connect.flags());
        VLOG(0) << "Protocol: " << connect.protocol();
        VLOG(0) << "Version: " << static_cast<uint16_t>(connect.version());

        sendConnack();
    }

    void SocketContext::onConnack(const ::mqtt::packets::Connack& connack) {
        VLOG(0) << "CONNACK";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(connack.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connack.getReserved());
        VLOG(0) << "RemainingLength: " << connack.getRemainingLength();
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connack.getFlags());
        VLOG(0) << "Reason: " << connack.getReason();
    }

    void SocketContext::onPublish(const ::mqtt::packets::Publish& publish) {
        VLOG(0) << "PUBLISH";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(publish.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(publish.getReserved());
        VLOG(0) << "RemainingLength: " << publish.getRemainingLength();
        VLOG(0) << "Topic: " << publish.getName();
        VLOG(0) << "Message: " << publish.getMessage();
        VLOG(0) << "PacketIdentifier: " << publish.getPacketIdentifier();

        //        sendPublishToAll(publish.getData());

        sendPuback(publish.getPacketIdentifier());
    }

    void SocketContext::onPuback(const ::mqtt::packets::Puback& puback) {
        VLOG(0) << "PUBACK";
        VLOG(0) << "======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(puback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(puback.getReserved());
        VLOG(0) << "RemainingLength: " << puback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(const ::mqtt::packets::Subscribe& subscribe) {
        VLOG(0) << "SUBSCRIBE";
        VLOG(0) << "=========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(subscribe.getReserved());
        VLOG(0) << "RemainingLength: " << subscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        std::list<uint8_t> returnCodes;

        for (const ::mqtt::Topic& topic : subscribe.getTopics()) {
            VLOG(0) << "Topic: " << topic.getName() << ", requestedQos: " << static_cast<uint16_t>(topic.getRequestedQos());
            returnCodes.push_back(0x00); // QoS = 0; Success
        }

        sendSuback(subscribe.getPacketIdentifier(), returnCodes);

        sendPublish(subscribe.getPacketIdentifier(), "hihi/hoho", "hallo");
    }

    void SocketContext::onSuback(const ::mqtt::packets::Suback& suback) {
        VLOG(0) << "SUBACK";
        VLOG(0) << "======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(suback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(suback.getReserved());
        VLOG(0) << "RemainingLength: " << suback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << suback.getPacketIdentifier();

        for (uint8_t returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  Return Code: " << static_cast<uint16_t>(returnCode);
        }
    }

    void SocketContext::onUnsubscribe(const ::mqtt::packets::Unsubscribe& unsubscribe) {
        VLOG(0) << "UNSUBSCRIBE";
        VLOG(0) << "===========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsubscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsubscribe.getReserved());
        VLOG(0) << "RemainingLength: " << unsubscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            VLOG(0) << "  Topic: " << topic;
        }
    }

    void SocketContext::onUnsuback(const ::mqtt::packets::Unsuback& unsuback) {
        VLOG(0) << "UNSUBACK";
        VLOG(0) << "========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsuback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsuback.getReserved());
        VLOG(0) << "RemainingLength: " << unsuback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsuback.getPacketIdentifier();
    }

    void SocketContext::onPingreq(const ::mqtt::packets::Pingreq& pingreq) {
        VLOG(0) << "PINGREQ";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingreq.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pingreq.getReserved());
        VLOG(0) << "RemainingLength: " << pingreq.getRemainingLength();
    }

    void SocketContext::onPingresp(const ::mqtt::packets::Pingresp& pingresp) {
        VLOG(0) << "PINGRESP";
        VLOG(0) << "========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingresp.getType());
        VLOG(0) << "RemainingLength: " << pingresp.getRemainingLength();
    }

    void SocketContext::onDisconnect(const ::mqtt::packets::Disconnect& disconnect) {
        VLOG(0) << "DISCONNECT";
        VLOG(0) << "==========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(disconnect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(disconnect.getReserved());
        VLOG(0) << "RemainingLength: " << disconnect.getRemainingLength();
    }

} // namespace apps::mqtt::server
