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

#include <algorithm>
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

                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "PacketType: " << static_cast<uint16_t>(staticHeader.getPacketType());
                LOG(TRACE) << "PacketFlags: " << static_cast<uint16_t>(staticHeader.getFlags());
                LOG(TRACE) << "RemainingLength: " << staticHeader.getRemainingLength();

                if (currentPacket == nullptr) {
                    LOG(TRACE) << "Received packet-type is unavailable ... closing connection";

                    close();
                    break;
                } else if (currentPacket->isError()) {
                    LOG(TRACE) << "Received packet-flags have error ... closing connection";

                    close();
                    break;
                }

                staticHeader.reset();

                state++;
                [[fallthrough]];
            case 1:
                if (currentPacket != nullptr) {
                    consumed += currentPacket->deserialize(this);

                    if (currentPacket->isComplete()) {
                        printData(currentPacket->serialize());
                        currentPacket->propagateEvent(this);

                        delete currentPacket;
                        currentPacket = nullptr;

                        state = 0;
                    } else if (currentPacket->isError()) {
                        delete currentPacket;
                        currentPacket = nullptr;

                        close();
                    }
                } else {
                    close();
                }

                break;
        }

        return consumed;
    }

    void SocketContext::_onConnect(packets::Connect& connect) {
        if (connect.getProtocol() != "MQTT") {
            close();
        } else if (connect.getLevel() != MQTT_VERSION_3_1_1) {
            sendConnack(MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);
            shutdown();
        } else if (connect.getClientId().empty() && !connect.getCleanSession()) {
            sendConnack(MQTT_CONNACK_IDENTIFIERREJECTED, MQTT_SESSION_NEW);
            shutdown();
        } else {
            if (connect.getClientId().empty()) {
                connect.setClientId(getRandomClientId());
            }

            onConnect(connect);
        }
    }

    void SocketContext::_onConnack(const packets::Connack& connack) {
        onConnack(connack);

        if (connack.getReturnCode() != MQTT_CONNACK_ACCEPT) {
            shutdown();
        }
    }

    void SocketContext::_onPublish(const packets::Publish& publish) {
        if (publish.getQoSLevel() > 2) {
            close();
        } else {
            onPublish(publish);

            switch (publish.getQoSLevel()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());
                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());
                    break;
                case 3:
                    LOG(TRACE) << "Received publish with QoS-Level 3 ... closing";
                    close();
                    break;
            }
        }
    }

    void SocketContext::_onPuback(const packets::Puback& puback) {
        onPuback(puback);
    }

    void SocketContext::_onPubrec(const packets::Pubrec& pubrec) {
        onPubrec(pubrec);

        sendPubrel(pubrec.getPacketIdentifier());
    }

    void SocketContext::_onPubrel(const packets::Pubrel& pubrel) {
        onPubrel(pubrel);

        sendPubcomp(pubrel.getPacketIdentifier());
    }

    void SocketContext::_onPubcomp(const packets::Pubcomp& pubcomp) {
        onPubcomp(pubcomp);
    }

    void SocketContext::_onSubscribe(const packets::Subscribe& subscribe) {
        bool wrongQos = std::any_of(subscribe.getTopics().begin(), subscribe.getTopics().end(), [](const Topic& topic) {
            return topic.getRequestedQoS() > 0x03;
        });

        if (wrongQos) {
            close();
        } else {
            onSubscribe(subscribe);

            std::list<uint8_t> returnCodes;

            for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
                returnCodes.push_back(topic.getRequestedQoS() | 0x00 /* 0x80 */); // QoS + Success
            }

            sendSuback(subscribe.getPacketIdentifier(), returnCodes);
        }
    }

    void SocketContext::_onSuback(const packets::Suback& suback) {
        onSuback(suback);
    }

    void SocketContext::_onUnsubscribe(const packets::Unsubscribe& unsubscribe) {
        onUnsubscribe(unsubscribe);

        sendUnsuback(unsubscribe.getPacketIdentifier());
    }

    void SocketContext::_onUnsuback(const packets::Unsuback& unsuback) {
        onUnsuback(unsuback);
    }

    void SocketContext::_onPingreq(const packets::Pingreq& pingreq) {
        onPingreq(pingreq);

        sendPingresp();
    }

    void SocketContext::_onPingresp(const packets::Pingresp& pingresp) {
        onPingresp(pingresp);
    }

    void SocketContext::_onDisconnect(const packets::Disconnect& disconnect) {
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
