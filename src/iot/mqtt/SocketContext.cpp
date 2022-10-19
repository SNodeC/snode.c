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

#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/packets/Puback.h"
#include "iot/mqtt/packets/Pubcomp.h"
#include "iot/mqtt/packets/Publish.h"
#include "iot/mqtt/packets/Pubrec.h"
#include "iot/mqtt/packets/Pubrel.h"

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
                consumed = fixedHeader.deserialize(this);

                if (!fixedHeader.isComplete()) {
                    break;
                } else if (fixedHeader.isError()) {
                    close();
                    break;
                }

                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "Fixed Header: PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getPacketType());
                LOG(TRACE) << "              PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getFlags()) << std::dec;
                LOG(TRACE) << "              RemainingLength: " << fixedHeader.getRemainingLength();

                controlPacketDeserializer = onReceiveFromPeer(fixedHeader);

                fixedHeader.reset();

                if (controlPacketDeserializer == nullptr) {
                    LOG(TRACE) << "Received packet-type is unavailable ... closing connection";

                    shutdown(true);
                    break;
                } else if (controlPacketDeserializer->isError()) {
                    LOG(TRACE) << "Fixed header has error ... closing connection";

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
                    //                    LOG(TRACE) << "Control packet ready ... propagating event";
                    propagateEvent(controlPacketDeserializer);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                } else if (controlPacketDeserializer->isError()) {
                    LOG(TRACE) << "Control packet has error ... closing connection";
                    shutdown(true);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                }

                break;
        }

        return consumed;
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
        const std::string& topic, const std::string& message, bool dup, uint8_t qoS, bool retain) { // Server & Client
        LOG(DEBUG) << "Send PUBLISH";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Publish(qoS == 0 ? 0 : getPacketIdentifier(), topic, message, dup, qoS, retain));
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

        LOG(DEBUG) << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getType());
        LOG(DEBUG) << "Flags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getFlags());
        LOG(DEBUG) << "RemainingLength: " << std::dec << dynamic_cast<const ControlPacketDeserializer&>(packet).getRemainingLength();
    }

    void SocketContext::printData(const std::vector<char>& data) {
        std::stringstream ss;

        ss << "Data: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
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
