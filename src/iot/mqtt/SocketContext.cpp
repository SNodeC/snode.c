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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <fstream>
#include <iomanip>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

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
                consumed = staticHeader.deserialize(this);

                if (!staticHeader.isComplete()) {
                    break;
                } else if (staticHeader.isError()) {
                    close();
                    break;
                }

                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(staticHeader.getPacketType());
                LOG(TRACE) << "PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(staticHeader.getFlags());
                LOG(TRACE) << "RemainingLength: " << staticHeader.getRemainingLength();

                switch (staticHeader.getPacketType()) {
                    case MQTT_CONNECT: // Server
                        currentPacket = new iot::mqtt::packets::Connect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_CONNACK: // Client
                        currentPacket = new iot::mqtt::packets::Connack(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PUBLISH: // Server & Client
                        currentPacket = new iot::mqtt::packets::Publish(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PUBACK: // Server & Client
                        currentPacket = new iot::mqtt::packets::Puback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PUBREC: // Server & Client
                        currentPacket = new iot::mqtt::packets::Pubrec(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PUBREL: // Server & Client
                        currentPacket = new iot::mqtt::packets::Pubrel(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PUBCOMP: // Server & Client
                        currentPacket = new iot::mqtt::packets::Pubcomp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_SUBSCRIBE: // Server
                        currentPacket = new iot::mqtt::packets::Subscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_SUBACK: // Client
                        currentPacket = new iot::mqtt::packets::Suback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_UNSUBSCRIBE: // Server
                        currentPacket = new iot::mqtt::packets::Unsubscribe(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_UNSUBACK: // Client
                        currentPacket = new iot::mqtt::packets::Unsuback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PINGREQ: // Server
                        currentPacket = new iot::mqtt::packets::Pingreq(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_PINGRESP: // Client
                        currentPacket = new iot::mqtt::packets::Pingresp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    case MQTT_DISCONNECT: // Server
                        currentPacket = new iot::mqtt::packets::Disconnect(staticHeader.getRemainingLength(), staticHeader.getFlags());
                        break;
                    default:
                        currentPacket = nullptr;
                        break;
                }

                staticHeader.reset();

                if (currentPacket == nullptr) {
                    LOG(TRACE) << "Received packet-type is unavailable ... closing connection";

                    shutdown(true);
                    break;
                } else if (currentPacket->isError()) {
                    LOG(TRACE) << "Static header has error ... closing connection";

                    delete currentPacket;
                    currentPacket = nullptr;

                    shutdown(true);
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += currentPacket->deserialize(this);

                if (currentPacket->isComplete()) {
                    printData(currentPacket->serialize());
                    currentPacket->propagateEvent(this);

                    delete currentPacket;
                    currentPacket = nullptr;

                    state = 0;
                } else if (currentPacket->isError()) {
                    shutdown(true);

                    delete currentPacket;
                    currentPacket = nullptr;

                    state = 0;
                }

                break;
        }

        return consumed;
    }

    void SocketContext::_onConnect(packets::Connect& connect) {
        if (connect.getProtocol() != "MQTT") {
            shutdown(true);
        } else if (connect.getLevel() != MQTT_VERSION_3_1_1) {
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);
            shutdown(true);

        } else if (connect.getClientId() == "" && !connect.getCleanSession()) {
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);
            shutdown(true);

        } else {
            if (connect.getClientId().empty()) {
                connect.setClientId(getRandomClientId());
            }

            onConnect(connect);
        }
    }

    void SocketContext::_onConnack(packets::Connack& connack) {
        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            shutdown(true);
        } else {
            onConnack(connack);
        }
    }

    void SocketContext::_onPublish(packets::Publish& publish) {
        if (publish.getQoSLevel() > 2) {
            shutdown(true);
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoSLevel() > 0) {
            shutdown(true);
        } else {
            switch (publish.getQoSLevel()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());
                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());
                    break;
            }

            onPublish(publish);
        }
    }

    void SocketContext::_onPuback(packets::Puback& puback) {
        if (puback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPuback(puback);
        }
    }

    void SocketContext::_onPubrec(packets::Pubrec& pubrec) {
        if (pubrec.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            sendPubrel(pubrec.getPacketIdentifier());

            onPubrec(pubrec);
        }
    }

    void SocketContext::_onPubrel(packets::Pubrel& pubrel) {
        if (pubrel.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            sendPubcomp(pubrel.getPacketIdentifier());

            onPubrel(pubrel);
        }
    }

    void SocketContext::_onPubcomp(packets::Pubcomp& pubcomp) {
        if (pubcomp.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onPubcomp(pubcomp);
        }
    }

    void SocketContext::_onSubscribe(packets::Subscribe& subscribe) {
        if (subscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSubscribe(subscribe); // Shall only subscribe but not send retained messages

            std::list<uint8_t> returnCodes;

            // Check QoS-Levels of subscribtions (topic.getRequestedQoS())
            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                returnCodes.push_back(topic.getAcceptedQoS()); // QoS + Success
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);

            // Here we shall trigger sending of retained messages
        }
    }

    void SocketContext::_onSuback(packets::Suback& suback) {
        if (suback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onSuback(suback);
        }
    }

    void SocketContext::_onUnsubscribe(packets::Unsubscribe& unsubscribe) {
        if (unsubscribe.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            sendUnsuback(unsubscribe.getPacketIdentifier());

            onUnsubscribe(unsubscribe);
        }
    }

    void SocketContext::_onUnsuback(packets::Unsuback& unsuback) {
        if (unsuback.getPacketIdentifier() == 0) {
            shutdown(true);
        } else {
            onUnsuback(unsuback);
        }
    }

    void SocketContext::_onPingreq(packets::Pingreq& pingreq) {
        sendPingresp();

        onPingreq(pingreq);
    }

    void SocketContext::_onPingresp(packets::Pingresp& pingresp) {
        onPingresp(pingresp);
    }

    void SocketContext::_onDisconnect(packets::Disconnect& disconnect) {
        onDisconnect(disconnect);

        shutdown();
    }

    void SocketContext::send(ControlPacket&& controlPacket) const {
        send(controlPacket.serialize());
    }

    void SocketContext::send(ControlPacket& controlPacket) const {
        send(controlPacket.serialize());
    }

    void SocketContext::send(std::vector<char>&& data) const {
        printData(data);

        sendToPeer(data.data(), data.size());
    }

    void SocketContext::sendConnect(const std::string& clientId) { // Client
        LOG(TRACE) << "Send CONNECT";
        LOG(TRACE) << "============";

        send(iot::mqtt::packets::Connect(clientId)); // Flags, Username, Will, ...
    }

    void SocketContext::sendConnack(uint8_t returnCode, uint8_t flags) { // Server
        LOG(TRACE) << "Send CONNACK";
        LOG(TRACE) << "============";

        send(iot::mqtt::packets::Connack(returnCode, flags));

        if (returnCode != MQTT_CONNACK_ACCEPT) {
            shutdown();
        }
    }

    void SocketContext::sendPublish(
        const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain) { // Server & Client
        LOG(TRACE) << "Send PUBLISH";
        LOG(TRACE) << "============";

        send(iot::mqtt::packets::Publish(qoSLevel == 0 ? 0 : getPacketIdentifier(), topic, message, dup, qoSLevel, retain));
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) { // Server & Client
        LOG(TRACE) << "Send PUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void SocketContext::sendPubrec(uint16_t packetIdentifier) { // Server & Client
        LOG(TRACE) << "Send PUBREC";
        LOG(TRACE) << "===========";

        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void SocketContext::sendPubrel(uint16_t packetIdentifier) { // Server & Client
        LOG(TRACE) << "Send PUBREL";
        LOG(TRACE) << "===========";

        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void SocketContext::sendPubcomp(uint16_t packetIdentifier) { // Server & Client
        LOG(TRACE) << "Send PUBCOMP";
        LOG(TRACE) << "============";

        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
    }

    void SocketContext::sendSubscribe(std::list<iot::mqtt::Topic>& topics) { // Client
        LOG(TRACE) << "Send SUBSCRIBE";
        LOG(TRACE) << "==============";

        send(iot::mqtt::packets::Subscribe(0, topics));
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) { // Server
        LOG(TRACE) << "Send SUBACK";
        LOG(TRACE) << "===========";

        send(iot::mqtt::packets::Suback(packetIdentifier, returnCodes));
    }

    void SocketContext::sendUnsubscribe(std::list<std::string>& topics) { // Client
        LOG(TRACE) << "Send UNSUBSCRIBE";
        LOG(TRACE) << "================";

        send(iot::mqtt::packets::Unsubscribe(getPacketIdentifier(), topics));
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) { // Server
        LOG(TRACE) << "Send UNSUBACK";
        LOG(TRACE) << "=============";

        send(iot::mqtt::packets::Unsuback(packetIdentifier));
    }

    void SocketContext::sendPingreq() { // Client
        LOG(TRACE) << "Send Pingreq";
        LOG(TRACE) << "============";

        send(iot::mqtt::packets::Pingreq());
    }

    void SocketContext::sendPingresp() { // Server
        LOG(TRACE) << "Send Pingresp";
        LOG(TRACE) << "=============";

        send(iot::mqtt::packets::Pingresp());
    }

    void SocketContext::sendDisconnect() { // Client
        LOG(TRACE) << "Send Disconnect";
        LOG(TRACE) << "===============";

        send(iot::mqtt::packets::Disconnect());

        shutdown();
    }

#define UUID_LEN 36

    std::string SocketContext::getRandomClientId() {
        char uuid[UUID_LEN];

        std::ifstream file("/proc/sys/kernel/random/uuid");
        file.getline(uuid, UUID_LEN);
        file.close();

        return std::string(uuid);
    }

    uint16_t SocketContext::getPacketIdentifier() {
        ++packetIdentifier;

        if (packetIdentifier == 0) {
            ++packetIdentifier;
        }

        return packetIdentifier;
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

} // namespace iot::mqtt
