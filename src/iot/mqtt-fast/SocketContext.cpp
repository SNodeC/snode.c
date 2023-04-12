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

#include "iot/mqtt-fast/SocketContext.h"

#include "iot/mqtt-fast/ControlPacket.h"
#include "iot/mqtt-fast/types/Binary.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection)
        , controlPacketFactory(this) {
    }

    void SocketContext::sendConnect(const std::string& clientId) {
        LOG(TRACE) << "Send CONNECT";
        LOG(TRACE) << "============";

        send(iot::mqtt_fast::packets::Connect(clientId));
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) {
        LOG(TRACE) << "Send CONNACK";
        LOG(TRACE) << "============";

        send(iot::mqtt_fast::packets::Connack(returnCode, flags));
    }

    void SocketContext::sendPublish(const std::string& topic, const std::string& message, bool dup, uint8_t qoS, bool retain) {
        LOG(TRACE) << "Send PUBLISH";
        LOG(TRACE) << "============";

        send(iot::mqtt_fast::packets::Publish(qoS == 0 ? 0 : getPacketIdentifier(), topic, message, dup, qoS, retain));
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt_fast::packets::Puback(packetIdentifier));
    }

    void SocketContext::sendPubrec(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBREC";
        LOG(TRACE) << "===========";

        send(iot::mqtt_fast::packets::Pubrec(packetIdentifier));
    }

    void SocketContext::sendPubrel(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBREL";
        LOG(TRACE) << "===========";

        send(iot::mqtt_fast::packets::Pubrel(packetIdentifier));
    }

    void SocketContext::sendPubcomp(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBCOMP";
        LOG(TRACE) << "============";

        send(iot::mqtt_fast::packets::Pubcomp(packetIdentifier));
    }

    void SocketContext::sendSubscribe(std::list<iot::mqtt_fast::Topic>& topics) {
        LOG(TRACE) << "Send SUBSCRIBE";
        LOG(TRACE) << "==============";

        send(iot::mqtt_fast::packets::Subscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        LOG(TRACE) << "Send SUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt_fast::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsubscribe(std::list<std::string>& topics) {
        LOG(TRACE) << "Send UNSUBSCRIBE";
        LOG(TRACE) << "================";

        send(iot::mqtt_fast::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send UNSUBACK";
        LOG(TRACE) << "=============";

        send(iot::mqtt_fast::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingreq() {
        LOG(TRACE) << "Send Pingreq";
        LOG(TRACE) << "============";

        send(iot::mqtt_fast::packets::Pingreq());
    }

    void SocketContext::sendPingresp() {
        LOG(TRACE) << "Send Pingresp";
        LOG(TRACE) << "=============";

        send(iot::mqtt_fast::packets::Pingresp());
    }

    void SocketContext::sendDisconnect() {
        LOG(TRACE) << "Send Disconnect";
        LOG(TRACE) << "===============";

        send(iot::mqtt_fast::packets::Disconnect());
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        std::size_t consumed = controlPacketFactory.construct();

        if (controlPacketFactory.isError()) {
            LOG(ERROR) << "SocketContext: Error during ControlPacket construction";
            close();
        } else if (controlPacketFactory.isComplete()) {
            LOG(TRACE) << "======================================================";
            LOG(TRACE) << "PacketType: " << static_cast<uint16_t>(controlPacketFactory.getPacketType());
            LOG(TRACE) << "PacketFlags: " << static_cast<uint16_t>(controlPacketFactory.getPacketFlags());
            LOG(TRACE) << "RemainingLength: " << static_cast<uint16_t>(controlPacketFactory.getRemainingLength());

            printData(controlPacketFactory.getPacket().getValue());

            switch (controlPacketFactory.getPacketType()) {
                case MQTT_CONNECT:
                    onConnect(iot::mqtt_fast::packets::Connect(controlPacketFactory));
                    break;
                case MQTT_CONNACK:
                    onConnack(iot::mqtt_fast::packets::Connack(controlPacketFactory));
                    break;
                case MQTT_PUBLISH:
                    onPublish(iot::mqtt_fast::packets::Publish(controlPacketFactory));
                    break;
                case MQTT_PUBACK:
                    onPuback(iot::mqtt_fast::packets::Puback(controlPacketFactory));
                    break;
                case MQTT_PUBREC:
                    onPubrec(iot::mqtt_fast::packets::Pubrec(controlPacketFactory));
                    break;
                case MQTT_PUBREL:
                    onPubrel(iot::mqtt_fast::packets::Pubrel(controlPacketFactory));
                    break;
                case MQTT_PUBCOMP:
                    onPubcomp(iot::mqtt_fast::packets::Pubcomp(controlPacketFactory));
                    break;
                case MQTT_SUBSCRIBE:
                    onSubscribe(iot::mqtt_fast::packets::Subscribe(controlPacketFactory));
                    break;
                case MQTT_SUBACK:
                    onSuback(iot::mqtt_fast::packets::Suback(controlPacketFactory));
                    break;
                case MQTT_UNSUBSCRIBE:
                    onUnsubscribe(iot::mqtt_fast::packets::Unsubscribe(controlPacketFactory));
                    break;
                case MQTT_UNSUBACK:
                    onUnsuback(iot::mqtt_fast::packets::Unsuback(controlPacketFactory));
                    break;
                case MQTT_PINGREQ:
                    onPingreq(iot::mqtt_fast::packets::Pingreq(controlPacketFactory));
                    break;
                case MQTT_PINGRESP:
                    onPingresp(iot::mqtt_fast::packets::Pingresp(controlPacketFactory));
                    break;
                case MQTT_DISCONNECT:
                    onDisconnect(iot::mqtt_fast::packets::Disconnect(controlPacketFactory));
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

    void SocketContext::send(std::vector<char>&& data) const {
        printData(data);
        sendToPeer(data.data(), data.size());
    }

    void SocketContext::printData(const std::vector<char>& data) const {
        std::stringstream ss;

        ss << "Data: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i + 1 != data.size()) {
                ss << std::endl;
                ss << "                                            ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }

        LOG(TRACE) << ss.str();
    }

} // namespace iot::mqtt_fast
