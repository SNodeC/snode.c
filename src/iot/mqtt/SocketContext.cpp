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

#include "iot/mqtt/SocketContext.h"

#include "iot/mqtt/ControlPacket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection)
        , controlPacketFactory(this) {
    }

    void SocketContext::onConnect(const mqtt::packets::Connect& connect) {
        VLOG(0) << "CONNECT";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(connect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connect.getReserved());
        VLOG(0) << "RemainingLength: " << connect.getRemainingLength();
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connect.getFlags());
        VLOG(0) << "Protocol: " << connect.getProtocol();
        VLOG(0) << "Version: " << static_cast<uint16_t>(connect.getVersion());

        sendConnack();
    }

    void SocketContext::onConnack(const mqtt::packets::Connack& connack) {
        VLOG(0) << "CONNACK";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(connack.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(connack.getReserved());
        VLOG(0) << "RemainingLength: " << connack.getRemainingLength();
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connack.getFlags());
        VLOG(0) << "Reason: " << connack.getReason();
    }

    void SocketContext::onPublish(const mqtt::packets::Publish& publish) {
        VLOG(0) << "PUBLISH";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(publish.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(publish.getReserved());
        VLOG(0) << "RemainingLength: " << publish.getRemainingLength();
        VLOG(0) << "Topic: " << publish.getTopic();
        VLOG(0) << "Message: " << publish.getMessage();
        VLOG(0) << "PacketIdentifier: " << publish.getPacketIdentifier();

        sendPuback(publish.getPacketIdentifier());
    }

    void SocketContext::onPuback(const mqtt::packets::Puback& puback) {
        VLOG(0) << "PUBACK";
        VLOG(0) << "======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(puback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(puback.getReserved());
        VLOG(0) << "RemainingLength: " << puback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(const mqtt::packets::Subscribe& subscribe) {
        VLOG(0) << "SUBSCRIBE";
        VLOG(0) << "=========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(subscribe.getReserved());
        VLOG(0) << "RemainingLength: " << subscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        std::list<uint8_t> returnCodes;

        for (const mqtt::Topic& topic : subscribe.getTopics()) {
            VLOG(0) << "Topic: " << topic.getName() << ", requestedQos: " << static_cast<uint16_t>(topic.getRequestedQos());
            returnCodes.push_back(0x00); // QoS = 0; Success
        }

        sendSuback(subscribe.getPacketIdentifier(), returnCodes);
    }

    void SocketContext::onSuback(const mqtt::packets::Suback& suback) {
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

    void SocketContext::onUnsubscribe(const mqtt::packets::Unsubscribe& unsubscribe) {
        VLOG(0) << "UNSUBSCRIBE";
        VLOG(0) << "===========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsubscribe.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsubscribe.getReserved());
        VLOG(0) << "RemainingLength: " << unsubscribe.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            VLOG(0) << "  Topic: " << topic;
        }

        sendUnsuback(unsubscribe.getPacketIdentifier());
    }

    void SocketContext::onUnsuback(const mqtt::packets::Unsuback& unsuback) {
        VLOG(0) << "UNSUBACK";
        VLOG(0) << "========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(unsuback.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(unsuback.getReserved());
        VLOG(0) << "RemainingLength: " << unsuback.getRemainingLength();
        VLOG(0) << "PacketIdentifier: " << unsuback.getPacketIdentifier();
    }

    void SocketContext::onPingreq(const mqtt::packets::Pingreq& pingreq) {
        VLOG(0) << "PINGREQ";
        VLOG(0) << "=======";
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingreq.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(pingreq.getReserved());
        VLOG(0) << "RemainingLength: " << pingreq.getRemainingLength();

        sendPingresp();
    }

    void SocketContext::onPingresp(const mqtt::packets::Pingresp& pingresp) {
        VLOG(0) << "PINGRESP";
        VLOG(0) << "========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(pingresp.getType());
        VLOG(0) << "RemainingLength: " << pingresp.getRemainingLength();
    }

    void SocketContext::onDisconnect(const mqtt::packets::Disconnect& disconnect) {
        VLOG(0) << "DISCONNECT";
        VLOG(0) << "==========";
        VLOG(0) << "Type: " << static_cast<uint16_t>(disconnect.getType());
        VLOG(0) << "Reserved: " << static_cast<uint16_t>(disconnect.getReserved());
        VLOG(0) << "RemainingLength: " << disconnect.getRemainingLength();

        close();
    }

    void SocketContext::sendConnect() {
        VLOG(0) << "Send CONNECT";
        VLOG(0) << "============";

        send(mqtt::packets::Connect("ClientId"));
    }

    void SocketContext::sendConnack() {
        VLOG(0) << "Send CONNACK";
        VLOG(0) << "============";

        send(mqtt::packets::Connack(MQTT_CONNECT_ACCEPT));
    }

    void SocketContext::sendPublish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain) {
        VLOG(0) << "Send PUBLISH";
        VLOG(0) << "============";

        send(iot::mqtt::packets::Publish(packetIdentifier, topic, message, dup, qoSLevel, retain));
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBACK";
        VLOG(0) << "===========";

        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void SocketContext::sendSubscribe(uint16_t packetIdentifier, std::list<iot::mqtt::Topic>& topics) {
        VLOG(0) << "Send SUBSCRIBE";
        VLOG(0) << "==============";

        send(iot::mqtt::packets::Subscribe(packetIdentifier, topics));
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        VLOG(0) << "Send SUBACK";
        VLOG(0) << "===========";

        send(iot::mqtt::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics) {
        VLOG(0) << "Send UNSUBSCRIBE";
        VLOG(0) << "================";

        send(iot::mqtt::packets::Unsubscribe(packetIdentifier, topics));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        VLOG(0) << "Send UNSUBACK";
        VLOG(0) << "=============";

        send(iot::mqtt::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingreq() {
        VLOG(0) << "Send Pingreq";
        VLOG(0) << "============";

        send(iot::mqtt::packets::Pingreq());
    }

    void SocketContext::sendPingresp() {
        VLOG(0) << "Send Pingresp";
        VLOG(0) << "=============";

        send(iot::mqtt::packets::Pingresp());
    }

    void SocketContext::sendDisconnect() {
        VLOG(0) << "Send Disconnect";
        VLOG(0) << "===============";

        send(iot::mqtt::packets::Disconnect());
    }

    /*
        void SocketContext::onPubrec(const mqtt::packets::Pubrec& pubrec) {
        }

        void SocketContext::onPubrel(const mqtt::packets::Pubrel& pubrel) {
        }

        void SocketContext::onPubcomp(const mqtt::packets::Pubcomp& pubcomp) {
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
                case MQTT_CONNECT:
                    onConnect(mqtt::packets::Connect(controlPacketFactory));
                    break;
                case MQTT_CONNACK:
                    onConnack(mqtt::packets::Connack(controlPacketFactory));
                    break;
                case MQTT_PUBLISH:
                    onPublish(mqtt::packets::Publish(controlPacketFactory));
                    break;
                case MQTT_PUBACK:
                    onPuback(mqtt::packets::Puback(controlPacketFactory));
                    break;
                    // case MQTT_PUBREC:
                    //     onPubrec(Pubrec(controlPacketFactory.get()));
                    break;
                    // case MQTT_PUBREL:
                    //     onPubrel(Pubrel(controlPacketFactory.get()));
                    break;
                    // case MQTT_PUBCOMP:
                    //     onPubcomp(Pubcomp(controlPacketFactory.get()));
                    break;
                case MQTT_SUBSCRIBE:
                    onSubscribe(mqtt::packets::Subscribe(controlPacketFactory));
                    break;
                case MQTT_SUBACK:
                    onSuback(mqtt::packets::Suback(controlPacketFactory));
                    break;
                case MQTT_UNSUBSCRIBE:
                    onUnsubscribe(mqtt::packets::Unsubscribe(controlPacketFactory));
                    break;
                case MQTT_UNSUBACK:
                    onUnsuback(mqtt::packets::Unsuback(controlPacketFactory));
                    break;
                case MQTT_PINGREQ:
                    onPingreq(mqtt::packets::Pingreq(controlPacketFactory));
                    break;
                case MQTT_PINGRESP:
                    onPingresp(mqtt::packets::Pingresp(controlPacketFactory));
                    break;
                case MQTT_DISCONNECT:
                    onDisconnect(mqtt::packets::Disconnect(controlPacketFactory));
                    break;
                    // case MQTT_AUTH:
                    //     onAuth(Auth(controlPacketFactory.get()));
                    break;
                default:
                    close();
                    break;
            }

            onControlPackageReceived(controlPacketFactory.getPacket());

            controlPacketFactory.reset();
        }

        return consumed;
    }

    void SocketContext::send(ControlPacket&& controlPacket) const {
        send(controlPacket.getPacket());
    }

    void SocketContext::send(ControlPacket& controlPacket) const {
        send(controlPacket.getPacket());
    }

    void SocketContext::send(const std::vector<char>& data) const {
        printData(data);
        sendToPeer(data.data(), data.size());
    }

    void SocketContext::printData(const std::vector<char>& data) const {
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

} // namespace iot::mqtt
