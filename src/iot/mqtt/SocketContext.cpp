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
        if (controlPacketDeserializer != nullptr) {
            delete controlPacketDeserializer;
            controlPacketDeserializer = nullptr;
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

                controlPacketDeserializer = onReceiveFromPeer(staticHeader);

                if (controlPacketDeserializer == nullptr) {
                    switch (staticHeader.getPacketType()) {
                        case MQTT_PUBLISH: // Server & Client
                            controlPacketDeserializer =
                                new iot::mqtt::packets::Publish(staticHeader.getRemainingLength(), staticHeader.getFlags());
                            break;
                        case MQTT_PUBACK: // Server & Client
                            controlPacketDeserializer =
                                new iot::mqtt::packets::Puback(staticHeader.getRemainingLength(), staticHeader.getFlags());
                            break;
                        case MQTT_PUBREC: // Server & Client
                            controlPacketDeserializer =
                                new iot::mqtt::packets::Pubrec(staticHeader.getRemainingLength(), staticHeader.getFlags());
                            break;
                        case MQTT_PUBREL: // Server & Client
                            controlPacketDeserializer =
                                new iot::mqtt::packets::Pubrel(staticHeader.getRemainingLength(), staticHeader.getFlags());
                            break;
                        case MQTT_PUBCOMP: // Server & Client
                            controlPacketDeserializer =
                                new iot::mqtt::packets::Pubcomp(staticHeader.getRemainingLength(), staticHeader.getFlags());
                            break;
                        default:
                            controlPacketDeserializer = nullptr;
                            break;
                    }
                }

                staticHeader.reset();

                if (controlPacketDeserializer == nullptr) {
                    LOG(TRACE) << "Received packet-type is unavailable ... closing connection";

                    shutdown(true);
                    break;
                } else if (controlPacketDeserializer->isError()) {
                    LOG(TRACE) << "Static header has error ... closing connection";

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    shutdown(true);
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += controlPacketDeserializer->deserialize(this);

                if (controlPacketDeserializer->isComplete()) {
                    controlPacketDeserializer->propagateEvent(this);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                } else if (controlPacketDeserializer->isError()) {
                    shutdown(true);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                }

                break;
        }

        return consumed;
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

            __onPublish(publish);
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

    void SocketContext::sendPublish(
        const std::string& topic, const std::string& message, bool dup, uint8_t qoSLevel, bool retain) { // Server & Client
        LOG(DEBUG) << "Send PUBLISH";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Publish(qoSLevel == 0 ? 0 : getPacketIdentifier(), topic, message, dup, qoSLevel, retain));
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) { // Server & Client
        LOG(DEBUG) << "Send PUBACK";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void SocketContext::sendPubrec(uint16_t packetIdentifier) { // Server & Client
        LOG(DEBUG) << "Send PUBREC";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void SocketContext::sendPubrel(uint16_t packetIdentifier) { // Server & Client
        LOG(DEBUG) << "Send PUBREL";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void SocketContext::sendPubcomp(uint16_t packetIdentifier) { // Server & Client
        LOG(DEBUG) << "Send PUBCOMP";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
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

    void SocketContext::printStandardHeader(const iot::mqtt::ControlPacket& packet) {
        printData(packet.serialize());

        LOG(DEBUG) << "Error: " << dynamic_cast<const ControlPacketDeserializer&>(packet).isError();
        LOG(DEBUG) << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getType());
        LOG(DEBUG) << "Flags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getFlags());
        LOG(DEBUG) << "RemainingLength: " << dynamic_cast<const ControlPacketDeserializer&>(packet).getRemainingLength();
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

        LOG(DEBUG) << ss.str();
    }

} // namespace iot::mqtt
