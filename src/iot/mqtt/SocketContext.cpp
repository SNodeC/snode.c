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

    void SocketContext::sendPubrec(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBREC";
        VLOG(0) << "===========";

        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void SocketContext::sendPubrel(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBREL";
        VLOG(0) << "===========";

        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void SocketContext::sendPubcomp(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBCOMP";
        VLOG(0) << "============";

        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
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
                    onConnect(iot::mqtt::packets::Connect(controlPacketFactory));
                    break;
                case MQTT_CONNACK:
                    onConnack(iot::mqtt::packets::Connack(controlPacketFactory));
                    break;
                case MQTT_PUBLISH:
                    onPublish(iot::mqtt::packets::Publish(controlPacketFactory));
                    break;
                case MQTT_PUBACK:
                    onPuback(iot::mqtt::packets::Puback(controlPacketFactory));
                    break;
                case MQTT_PUBREC:
                    onPubrec(iot::mqtt::packets::Pubrec(controlPacketFactory));
                    break;
                case MQTT_PUBREL:
                    onPubrel(iot::mqtt::packets::Pubrel(controlPacketFactory));
                    break;
                case MQTT_PUBCOMP:
                    onPubcomp(iot::mqtt::packets::Pubcomp(controlPacketFactory));
                    break;
                case MQTT_SUBSCRIBE:
                    onSubscribe(iot::mqtt::packets::Subscribe(controlPacketFactory));
                    break;
                case MQTT_SUBACK:
                    onSuback(iot::mqtt::packets::Suback(controlPacketFactory));
                    break;
                case MQTT_UNSUBSCRIBE:
                    onUnsubscribe(iot::mqtt::packets::Unsubscribe(controlPacketFactory));
                    break;
                case MQTT_UNSUBACK:
                    onUnsuback(iot::mqtt::packets::Unsuback(controlPacketFactory));
                    break;
                case MQTT_PINGREQ:
                    onPingreq(iot::mqtt::packets::Pingreq(controlPacketFactory));
                    break;
                case MQTT_PINGRESP:
                    onPingresp(iot::mqtt::packets::Pingresp(controlPacketFactory));
                    break;
                case MQTT_DISCONNECT:
                    onDisconnect(iot::mqtt::packets::Disconnect(controlPacketFactory));
                    break;
                default:
                    close();
                    break;
            }

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
