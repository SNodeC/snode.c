/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "Mqtt.h"

#include "MqttContext.h"
#include "core/socket/SocketConnection.h"
#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/packets/Puback.h"
#include "iot/mqtt/packets/Pubcomp.h"
#include "iot/mqtt/packets/Publish.h"
#include "iot/mqtt/packets/Pubrec.h"
#include "iot/mqtt/packets/Pubrel.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <iomanip>
#include <ostream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    Mqtt::~Mqtt() {
        if (controlPacketDeserializer != nullptr) {
            delete controlPacketDeserializer;
            controlPacketDeserializer = nullptr;
        }

        keepAliveTimer.cancel();
    }

    void Mqtt::setMqttContext(MqttContext* mqttContext) {
        this->mqttContext = mqttContext;
    }

    void Mqtt::onConnected() {
    }

    std::size_t Mqtt::onReceiveFromPeer() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = fixedHeader.deserialize(mqttContext);

                if (!fixedHeader.isComplete()) {
                    break;
                } else if (fixedHeader.isError()) {
                    mqttContext->kill();
                    break;
                }

                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "Fixed Header: PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getPacketType());
                LOG(TRACE) << "              PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getFlags()) << std::dec;
                LOG(TRACE) << "              RemainingLength: " << fixedHeader.getRemainingLength();

                controlPacketDeserializer = createControlPacketDeserializer(fixedHeader);

                fixedHeader.reset();

                if (controlPacketDeserializer == nullptr) {
                    LOG(TRACE) << "Received packet-type is unavailable ... closing connection";

                    mqttContext->end(true);
                    break;
                } else if (controlPacketDeserializer->isError()) {
                    LOG(TRACE) << "Fixed header has error ... closing connection";

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    mqttContext->end(true);
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                consumed += controlPacketDeserializer->deserialize(mqttContext);

                if (controlPacketDeserializer->isComplete()) {
                    propagateEvent(controlPacketDeserializer);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;

                    keepAliveTimer.restart();
                } else if (controlPacketDeserializer->isError()) {
                    LOG(TRACE) << "Control packet has error ... closing connection";
                    mqttContext->end(true);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;
                }

                break;
        }

        return consumed;
    }

    void Mqtt::onDisconnected() {
    }

    void Mqtt::onExit() {
    }

    core::socket::SocketConnection* Mqtt::getSocketConnection() {
        return mqttContext->getSocketConnection();
    }

    void Mqtt::send(ControlPacket&& controlPacket) const {
        send(controlPacket.serialize());
    }

    void Mqtt::send(ControlPacket& controlPacket) const {
        send(controlPacket.serialize());
    }

    void Mqtt::send(std::vector<char>&& data) const {
        LOG(DEBUG) << dataToHexString(data);

        mqttContext->send(data.data(), data.size());
    }

    void Mqtt::sendPublish(const std::string& topic, const std::string& message, uint8_t qoS, bool dup,
                           bool retain) { // Server & Client
        LOG(DEBUG) << "Send PUBLISH";
        LOG(DEBUG) << "============";

        uint16_t pId = getPacketIdentifier();
        iot::mqtt::packets::Publish publish(qoS == 0 ? 0 : pId, topic, message, qoS, dup, retain);

        if (qoS == 2) {
            packetMap[pId] = publish;
        }

        send(publish);
    }

    void Mqtt::sendPuback(uint16_t packetIdentifier) const { // Server & Client
        LOG(DEBUG) << "Send PUBACK";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void Mqtt::sendPubrec(uint16_t packetIdentifier) const { // Server & Client
        LOG(DEBUG) << "Send PUBREC";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void Mqtt::sendPubrel(uint16_t packetIdentifier) const { // Server & Client
        LOG(DEBUG) << "Send PUBREL";
        LOG(DEBUG) << "===========";

        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void Mqtt::sendPubcomp(uint16_t packetIdentifier) const { // Server & Client
        LOG(DEBUG) << "Send PUBCOMP";
        LOG(DEBUG) << "============";

        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
    }

    void Mqtt::onPubrec(const packets::Pubrec& pubrec) {
        packetMap.erase(pubrec.getPacketIdentifier());
        packetIdentifierSet.insert(pubrec.getPacketIdentifier());
    }

    void Mqtt::onPubcomp(const packets::Pubcomp& pubcomp) {
        packetMap.erase(pubcomp.getPacketIdentifier());
        packetIdentifierSet.erase(pubcomp.getPacketIdentifier());
    }

    void Mqtt::printStandardHeader(const iot::mqtt::ControlPacket& packet) {
        LOG(DEBUG) << dataToHexString(packet.serialize());

        LOG(DEBUG) << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getType());
        LOG(DEBUG) << "Flags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getFlags());
        LOG(DEBUG) << "RemainingLength: " << std::dec << dynamic_cast<const ControlPacketDeserializer&>(packet).getRemainingLength();
    }

    std::string Mqtt::dataToHexString(const std::vector<char>& data) {
        std::stringstream ss;

        ss << "Data: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
                ss << std::endl;
                ss << "                                               ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }

        return ss.str();
    }

    void Mqtt::setKeepAlive(const utils::Timeval& timeout) {
        if (timeout > 0) {
            keepAliveTimer = core::timer::Timer::singleshotTimer(
                [this, timeout]() -> void {
                    LOG(TRACE) << "Keep-alive timer expired. Interval was: " << timeout;
                    mqttContext->kill();
                },
                timeout);

            getSocketConnection()->setTimeout(0);
        }
    }

    uint16_t Mqtt::getPacketIdentifier() {
        ++packetIdentifier;

        if (packetIdentifier == 0) {
            ++packetIdentifier;
        }

        return packetIdentifier;
    }

} // namespace iot::mqtt
