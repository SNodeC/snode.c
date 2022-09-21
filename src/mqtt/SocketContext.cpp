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

#include "mqtt/SocketContext.h"

#include "mqtt/Topic.h"
#include "mqtt/packets/Connect.h"
#include "mqtt/packets/Publish.h"
#include "mqtt/packets/Subscribe.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <endian.h>
#include <iomanip>
#include <iostream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection)
        , controlPacketFactory(this) {
    }

    void SocketContext::onConnect(const packets::Connect& connect) {
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

    void SocketContext::onPublish(const packets::Publish& publish) {
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

    void SocketContext::onSubscribe(const mqtt::packets::Subscribe& subscribe) {
        VLOG(0) << "SUBSCRIBE";
        VLOG(0) << "=========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(subscribe.getReserved());
        VLOG(0) << "RemainingLength: " << subscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        std::list<uint8_t> returnCodes;

        for (const Topic& topic : subscribe.getTopics()) {
            VLOG(0) << "Topic: " << topic.getName() << ", requestedQos: " << static_cast<uint16_t>(topic.getRequestedQos());
            returnCodes.push_back(0x00); // QoS = 0; Success
        }

        sendSuback(subscribe.getPacketIdentifier(), returnCodes);
    }

    void SocketContext::sendConnack() {
        VLOG(0) << "Send CONNACK";
        VLOG(0) << "============";

        std::vector<char> data;

        data.push_back(0x20); // Connack Type: 0x02, Flags: 0x00
        data.push_back(0x03); // Remaining length
        data.push_back(0x00); // Connack Flags: 0x00 - LSB is session present
        data.push_back(0x00); // Connack Reason: 0x00 = Success
        data.push_back(0x00); // Connack Property Length: 0x00 - no properties

        send(data);
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBACK";
        VLOG(0) << "===========";

        std::vector<char> data;

        data.push_back(0x40); // Puback Type: 0x04
        data.push_back(0x02); // Remaining length

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        send(data);
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        VLOG(0) << "Send SUBACK";
        VLOG(0) << "===========";

        std::vector<char> data;

        data.push_back(static_cast<char>(0x90));                      // Suback Type: 0x09
        data.push_back(static_cast<char>(0x02 + returnCodes.size())); // Remaining length

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        for (uint8_t returnCode : returnCodes) {
            data.push_back(static_cast<char>(returnCode));
        }

        send(data);
    }

    /*
        void SocketContext::onConnack(const mqtt::packets::Connack& connack) {
        }

        void SocketContext::onPublish(const mqtt::packets::Publish& publish) {
        }

        void SocketContext::onPuback(const mqtt::packets::Puback& puback) {
        }

        void SocketContext::onPubrec(const mqtt::packets::Pubrec& pubrec) {
        }

        void SocketContext::onPubrel(const mqtt::packets::Pubrel& pubrel) {
        }

        void SocketContext::onPubcomp(const mqtt::packets::Pubcomp& pubcomp) {
        }

        void SocketContext::onSubscribe(const mqtt::packets::Subscribe& subscribe) {
        }

        void SocketContext::onSuback(const mqtt::packets::Suback& suback) {
        }

        void SocketContext::onUnsubscribe(const mqtt::packets::Unsubscribe& unsubscribe) {
        }

        void SocketContext::onUnsuback(const mqtt::packets::Unsuback& unsuback) {
        }

        void SocketContext::onPingreq(const mqtt::packets::Pingreq& pingreq) {
        }

        void SocketContext::onPingresp(const mqtt::packets::Pingresp& pingresp) {
        }

        void SocketContext::onDisconnect(const mqtt::packets::Disconnect& disconnect) {
        }

        void SocketContext::onAuth(const mqtt::packets::Auth& auth){

        }
    */

    std::size_t SocketContext::onReceiveFromPeer() {
        std::size_t consumed = controlPacketFactory.construct();

        if (controlPacketFactory.isError()) {
            VLOG(0) << "SocketContext: Error during ControlPacket construction";
            close();
        } else if (controlPacketFactory.isComplete()) {
            VLOG(0) << "======================================================";
            VLOG(0) << "PacketType: " << static_cast<uint16_t>(controlPacketFactory.getPacketType());
            VLOG(0) << "RemainingLength: " << static_cast<uint16_t>(controlPacketFactory.getRemainingLength());
            printData(controlPacketFactory.getPacket());
            switch (controlPacketFactory.getPacketType()) {
                case 0x01:
                    onConnect(mqtt::packets::Connect(controlPacketFactory));
                    break;
                case 0x02:
                    // onConnack(Conack(controlPacketFactory.get()));
                    break;
                case 0x03:
                    onPublish(mqtt::packets::Publish(controlPacketFactory));
                    break;
                case 0x04:
                    // onPuback(Puback(controlPacketFactory.get()));
                    break;
                case 0x05:
                    // onPubrec(Pubrec(controlPacketFactory.get()));
                    break;
                case 0x06:
                    // onPubrel(Pubrel(controlPacketFactory.get()));
                    break;
                case 0x07:
                    // onPubcomp(Pubcomp(controlPacketFactory.get()));
                    break;
                case 0x08:
                    onSubscribe(mqtt::packets::Subscribe(controlPacketFactory));
                    break;
                case 0x09:
                    // onSuback(Suback(controlPacketFactory.get()));
                    break;
                case 0x0A:
                    // onUnsubscribe(Unsubscribe(controlPacketFactory.get()));
                    break;
                case 0x0B:
                    // onUnsuback(Unsuback(controlPacketFactory.get()));
                    break;
                case 0x0C:
                    // onPingreq(Pingreq(controlPacketFactory.get()));
                    break;
                case 0x0D:
                    // onPingresp(Pingresp(controlPacketFactory.get()));
                    break;
                case 0x0E:
                    // onDisconnect(Disconnect(controlPacketFactory.get()));
                    break;
                case 0x0F:
                    // onAuth(Auth(controlPacketFactory.get()));
                    break;
                default:
                    // onUnknown(Unknown(controlPacketFactory.get()));
                    break;
            }

            onControlPackageReceived(controlPacketFactory.getPacket());

            controlPacketFactory.reset();
        }

        return consumed;
    }

    void SocketContext::printData(std::vector<char>& data) {
        std::cout << "                                Data: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i + 1 != data.size()) {
                std::cout << std::endl;
                std::cout << "                                      ";
                // "| ";
            }
            ++i;
            std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
                      << " "; // << " | ";
        }
        std::cout << std::endl;
    }

    void SocketContext::send(std::vector<char>& data) {
        printData(data);
        sendToPeer(data.data(), data.size());
    }

} // namespace mqtt
