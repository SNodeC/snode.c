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

#include "iot/mqtt1/SocketContext.h"

#include "iot/mqtt1/ControlPacket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection) {
    }

    SocketContext::~SocketContext() {
        if (currentPacket != nullptr) {
            delete currentPacket;
            currentPacket = nullptr;
        }
    }

    std::size_t SocketContext::onReceiveFromPeer() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = controlPacketFactory.construct(this);

                error = controlPacketFactory.isError();
                complete = controlPacketFactory.isComplete();
                if (error || !complete) {
                    break;
                }

                switch (controlPacketFactory.getPacketType()) {
                    case MQTT_CONNECT:
                        currentPacket = new iot::mqtt1::packets::Connect(controlPacketFactory.getRemainingLength(),
                                                                         controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_CONNACK:
                        currentPacket = new iot::mqtt1::packets::Connack(controlPacketFactory.getRemainingLength(),
                                                                         controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_SUBSCRIBE:
                        currentPacket = new iot::mqtt1::packets::Subscribe(controlPacketFactory.getRemainingLength(),
                                                                           controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_SUBACK:
                        currentPacket = new iot::mqtt1::packets::Suback(controlPacketFactory.getRemainingLength(),
                                                                        controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PUBLISH:
                        currentPacket = new iot::mqtt1::packets::Publish(controlPacketFactory.getRemainingLength(),
                                                                         controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PUBREC:
                        currentPacket = new iot::mqtt1::packets::Pubrec(controlPacketFactory.getRemainingLength(),
                                                                        controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PUBREL:
                        currentPacket = new iot::mqtt1::packets::Pubrel(controlPacketFactory.getRemainingLength(),
                                                                        controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_UNSUBSCRIBE:
                        currentPacket = new iot::mqtt1::packets::Unsubscribe(controlPacketFactory.getRemainingLength(),
                                                                             controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_UNSUBACK:
                        currentPacket = new iot::mqtt1::packets::Unsuback(controlPacketFactory.getRemainingLength(),
                                                                          controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PUBACK:
                        currentPacket = new iot::mqtt1::packets::Puback(controlPacketFactory.getRemainingLength(),
                                                                        controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PUBCOMP:
                        currentPacket = new iot::mqtt1::packets::Pubcomp(controlPacketFactory.getRemainingLength(),
                                                                         controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_DISCONNECT:
                        currentPacket = new iot::mqtt1::packets::Disconnect(controlPacketFactory.getRemainingLength(),
                                                                            controlPacketFactory.getPacketFlags());
                        break;
                    case MQTT_PINGREQ:
                        currentPacket = new iot::mqtt1::packets::Pingreq(controlPacketFactory.getRemainingLength(),
                                                                         controlPacketFactory.getPacketFlags());
                        break;
                }

                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "PacketType: " << static_cast<uint16_t>(controlPacketFactory.getPacketType());
                LOG(TRACE) << "PacketFlags: " << static_cast<uint16_t>(controlPacketFactory.getPacketFlags());
                LOG(TRACE) << "RemainingLength: " << static_cast<uint16_t>(controlPacketFactory.getRemainingLength());

                state++;
                [[fallthrough]];
            case 1:
                if (currentPacket != nullptr) {
                    consumed += currentPacket->_construct(this);

                    error = currentPacket->isError();
                    complete = currentPacket->isComplete();

                    if (complete) {
                        printData(currentPacket->getFullPacket());
                        currentPacket->propagateEvent(this);

                        delete currentPacket;
                        currentPacket = nullptr;

                        controlPacketFactory.reset();

                        state = 0;
                    }
                    if (error) {
                        delete currentPacket;
                        currentPacket = nullptr;

                        close();
                    }
                } else {
                    close();
                }

                break;
        }

        if (error) {
            close();
        }

        return consumed;
    }

    void SocketContext::_onConnect(const packets::Connect& connect) {
        onConnect(connect);
    }

    void SocketContext::_onConnack(const packets::Connack& connack) {
        onConnack(connack);
    }

    void SocketContext::_onPublish(const packets::Publish& publish) {
        onPublish(publish);
    }

    void SocketContext::_onPuback(const packets::Puback& puback) {
        onPuback(puback);
    }

    void SocketContext::_onPubrec(const packets::Pubrec& pubrec) {
        onPubrec(pubrec);
    }

    void SocketContext::_onPubrel(const packets::Pubrel& pubrel) {
        onPubrel(pubrel);
    }

    void SocketContext::_onPubcomp(const packets::Pubcomp& pubcomp) {
        onPubcomp(pubcomp);
    }

    void SocketContext::_onSubscribe(const packets::Subscribe& subscribe) {
        onSubscribe(subscribe);
    }

    void SocketContext::_onSuback(const packets::Suback& suback) {
        onSuback(suback);
    }

    void SocketContext::_onUnsubscribe(const packets::Unsubscribe& unsubscribe) {
        onUnsubscribe(unsubscribe);
    }

    void SocketContext::_onUnsuback(const packets::Unsuback& unsuback) {
        onUnsuback(unsuback);
    }

    void SocketContext::_onPingreq(const packets::Pingreq& pingreq) {
        onPingreq(pingreq);
    }

    void SocketContext::_onPingresp(const packets::Pingresp& pingresp) {
        onPingresp(pingresp);
    }

    void SocketContext::_onDisconnect(const packets::Disconnect& disconnect) {
        onDisconnect(disconnect);
    }

    void SocketContext::send(ControlPacket&& controlPacket) const {
        send(controlPacket.getFullPacket());
    }

    void SocketContext::send(ControlPacket& controlPacket) const {
        send(controlPacket.getFullPacket());
    }

    void SocketContext::send(std::vector<char>&& data) const {
        printData(data);
        sendToPeer(data.data(), data.size());
    }

    void SocketContext::sendConnect(const std::string& clientId) {
        LOG(TRACE) << "Send CONNECT";
        LOG(TRACE) << "============";

        send(iot::mqtt1::packets::Connect(clientId)); // Flags, Username, Will, ...
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) {
        LOG(TRACE) << "Send CONNACK";
        LOG(TRACE) << "============";

        send(iot::mqtt1::packets::Connack(returnCode, flags));
    }

    void SocketContext::sendPublish(const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain) {
        LOG(TRACE) << "Send PUBLISH";
        LOG(TRACE) << "============";

        send(iot::mqtt1::packets::Publish(qoSLevel == 0 ? 0 : getPacketIdentifier(), topic, message, dup, qoSLevel, retain));
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt1::packets::Puback(packetIdentifier));
    }

    void SocketContext::sendPubrec(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBREC";
        LOG(TRACE) << "===========";

        send(iot::mqtt1::packets::Pubrec(packetIdentifier));
    }

    void SocketContext::sendPubrel(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBREL";
        LOG(TRACE) << "===========";

        send(iot::mqtt1::packets::Pubrel(packetIdentifier));
    }

    void SocketContext::sendPubcomp(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send PUBCOMP";
        LOG(TRACE) << "============";

        send(iot::mqtt1::packets::Pubcomp(packetIdentifier));
    }

    void SocketContext::sendSubscribe(std::list<iot::mqtt1::Topic>& topics) {
        LOG(TRACE) << "Send SUBSCRIBE";
        LOG(TRACE) << "==============";

        send(iot::mqtt1::packets::Subscribe(0, topics));
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        LOG(TRACE) << "Send SUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt1::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsubscribe(std::list<std::string>& topics) {
        LOG(TRACE) << "Send UNSUBSCRIBE";
        LOG(TRACE) << "================";

        send(iot::mqtt1::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        LOG(TRACE) << "Send UNSUBACK";
        LOG(TRACE) << "=============";

        send(iot::mqtt1::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingreq() {
        LOG(TRACE) << "Send Pingreq";
        LOG(TRACE) << "============";

        send(iot::mqtt1::packets::Pingreq());
    }

    void SocketContext::sendPingresp() {
        LOG(TRACE) << "Send Pingresp";
        LOG(TRACE) << "=============";

        send(iot::mqtt1::packets::Pingresp());
    }

    void SocketContext::sendDisconnect() {
        LOG(TRACE) << "Send Disconnect";
        LOG(TRACE) << "===============";

        send(iot::mqtt1::packets::Disconnect());
    }

    void SocketContext::printData(const std::vector<char>& data) {
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

    uint16_t SocketContext::getPacketIdentifier() {
        ++packetIdentifier;

        if (packetIdentifier == 0) {
            ++packetIdentifier;
        }

        return packetIdentifier;
    }

} // namespace iot::mqtt1
