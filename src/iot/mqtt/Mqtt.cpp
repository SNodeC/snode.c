#include <SemanticLog.h>
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
#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/Session.h"
#include "iot/mqtt/packets/Puback.h"
#include "iot/mqtt/packets/Pubcomp.h"
#include "iot/mqtt/packets/Publish.h"
#include "iot/mqtt/packets/Pubrec.h"
#include "iot/mqtt/packets/Pubrel.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <iomanip>
#include <ostream>
#include <set>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

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

    std::size_t Mqtt::onProcess() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = fixedHeader.deserialize(mqttContext);

                if (!fixedHeader.isComplete()) {
                    break;
                } else if (fixedHeader.isError()) {
                    mqttContext->close();
                    break;
                }

                snode::semantic::appLog().trace() << "======================================================";
                snode::semantic::appLog().trace() << "Fixed Header: PacketType: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getPacketType());
                snode::semantic::appLog().trace() << "              PacketFlags: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<uint16_t>(fixedHeader.getFlags()) << std::dec;
                snode::semantic::appLog().trace() << "              RemainingLength: " << fixedHeader.getRemainingLength();

                controlPacketDeserializer = createControlPacketDeserializer(fixedHeader);

                fixedHeader.reset();

                if (controlPacketDeserializer == nullptr) {
                    snode::semantic::appLog().trace() << "Received packet-type is unavailable ... closing connection";

                    mqttContext->end(true);
                    break;
                } else if (controlPacketDeserializer->isError()) {
                    snode::semantic::appLog().trace() << "Fixed header has error ... closing connection";

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
                    deliverPacket(controlPacketDeserializer);

                    delete controlPacketDeserializer;
                    controlPacketDeserializer = nullptr;

                    state = 0;

                    keepAliveTimer.restart();
                } else if (controlPacketDeserializer->isError()) {
                    snode::semantic::appLog().trace() << "Control packet has error ... closing connection";
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

    core::socket::stream::SocketConnection* Mqtt::getSocketConnection() {
        return mqttContext->getSocketConnection();
    }

    void Mqtt::initSession(Session* session, const utils::Timeval& keepAlive) {
        this->session = session;

        for (auto& [packetIdentifier, publish] : session->publishMap) {
            snode::semantic::appLog().debug() << "Resend PUBLISH";
            snode::semantic::appLog().debug() << "==============";

            send(publish);
        }

        for (uint16_t packetIdentifier : session->pubrelPacketIdentifierSet) {
            snode::semantic::appLog().debug() << "Resend PUBREL";
            snode::semantic::appLog().debug() << "=============";

            send(iot::mqtt::packets::Pubrel(packetIdentifier));
        }

        if (keepAlive > 0) {
            keepAliveTimer = core::timer::Timer::singleshotTimer(
                [this, keepAlive]() -> void {
                    snode::semantic::appLog().trace() << "Keep-alive timer expired. Interval was: " << keepAlive;
                    mqttContext->close();
                },
                keepAlive);

            getSocketConnection()->setTimeout(0);
        }
    }

    void Mqtt::send(const ControlPacket& controlPacket) const {
        send(controlPacket.serialize());
    }

    void Mqtt::send(const std::vector<char>& data) const {
        snode::semantic::appLog().debug() << dataToHexString(data);

        mqttContext->send(data.data(), data.size());
    }

    void Mqtt::sendPublish(const std::string& topic, const std::string& message, uint8_t qoS,
                           bool retain) { // Server & Client
        snode::semantic::appLog().debug() << "Send PUBLISH";
        snode::semantic::appLog().debug() << "============";

        uint16_t pId = qoS != 0 ? getPacketIdentifier() : 0;

        send(iot::mqtt::packets::Publish(pId, topic, message, qoS, false, retain));

        if (qoS == 2) {
            session->publishMap.emplace(pId, iot::mqtt::packets::Publish(pId, topic, message, qoS, true, retain));
        }
    }

    void Mqtt::sendPuback(uint16_t packetIdentifier) const { // Server & Client
        snode::semantic::appLog().debug() << "Send PUBACK";
        snode::semantic::appLog().debug() << "===========";

        send(iot::mqtt::packets::Puback(packetIdentifier));
    }

    void Mqtt::sendPubrec(uint16_t packetIdentifier) const { // Server & Client
        snode::semantic::appLog().debug() << "Send PUBREC";
        snode::semantic::appLog().debug() << "===========";

        send(iot::mqtt::packets::Pubrec(packetIdentifier));
    }

    void Mqtt::sendPubrel(uint16_t packetIdentifier) const { // Server & Client
        snode::semantic::appLog().debug() << "Send PUBREL";
        snode::semantic::appLog().debug() << "===========";

        send(iot::mqtt::packets::Pubrel(packetIdentifier));
    }

    void Mqtt::sendPubcomp(uint16_t packetIdentifier) const { // Server & Client
        snode::semantic::appLog().debug() << "Send PUBCOMP";
        snode::semantic::appLog().debug() << "============";

        send(iot::mqtt::packets::Pubcomp(packetIdentifier));
    }

    void Mqtt::onPublish([[maybe_unused]] const packets::Publish& publish) {
    }

    void Mqtt::onPuback([[maybe_unused]] const iot::mqtt::packets::Puback& puback) {
    }

    void Mqtt::onPubrec([[maybe_unused]] const iot::mqtt::packets::Pubrec& pubrec) {
    }

    void Mqtt::onPubrel([[maybe_unused]] const iot::mqtt::packets::Pubrel& pubrel) {
    }

    void Mqtt::onPubcomp([[maybe_unused]] const iot::mqtt::packets::Pubcomp& pubcomp) {
    }

    bool Mqtt::_onPublish(const packets::Publish& publish) {
        bool deliver = true;
        printStandardHeader(publish);
        snode::semantic::appLog().debug() << "Topic: " << publish.getTopic();
        snode::semantic::appLog().debug() << "Message: " << publish.getMessage();
        snode::semantic::appLog().debug() << "QoS: " << static_cast<uint16_t>(publish.getQoS());
        snode::semantic::appLog().debug() << "PacketIdentifier: " << publish.getPacketIdentifier();
        snode::semantic::appLog().debug() << "DUP: " << publish.getDup();
        snode::semantic::appLog().debug() << "Retain: " << publish.getRetain();

        if (publish.getQoS() > 2) {
            snode::semantic::appLog().trace() << "Received invalid QoS: " << publish.getQoS();
            mqttContext->end(true);
            deliver = false;
        } else if (publish.getPacketIdentifier() == 0 && publish.getQoS() > 0) {
            snode::semantic::appLog().trace() << "Received QoS > 0 but no PackageIdentifier present";
            mqttContext->end(true);
            deliver = false;
        } else {
            switch (publish.getQoS()) {
                case 1:
                    sendPuback(publish.getPacketIdentifier());

                    break;
                case 2:
                    sendPubrec(publish.getPacketIdentifier());

                    if (session->publishPacketIdentifierSet.contains(publish.getPacketIdentifier())) {
                        deliver = false;
                    } else {
                        session->publishPacketIdentifierSet.insert(publish.getPacketIdentifier());
                    }

                    break;
            }
        }

        return deliver;
    }
    void Mqtt::_onPuback(const iot::mqtt::packets::Puback& puback) {
        snode::semantic::appLog().debug() << "Received PUBACK:";
        snode::semantic::appLog().debug() << "================";
        printStandardHeader(puback);
        snode::semantic::appLog().debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << puback.getPacketIdentifier();

        if (puback.getPacketIdentifier() == 0) {
            snode::semantic::appLog().trace() << "PackageIdentifier missing";
            mqttContext->end(true);
        }

        onPuback(puback);
    }

    void Mqtt::_onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        snode::semantic::appLog().debug() << "Received PUBREC:";
        snode::semantic::appLog().debug() << "================";
        printStandardHeader(pubrec);
        snode::semantic::appLog().debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrec.getPacketIdentifier();

        if (pubrec.getPacketIdentifier() == 0) {
            snode::semantic::appLog().trace() << "PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            session->publishMap.erase(pubrec.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.insert(pubrec.getPacketIdentifier());

            sendPubrel(pubrec.getPacketIdentifier());
        }

        onPubrec(pubrec);
    }

    void Mqtt::_onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        snode::semantic::appLog().debug() << "Received PUBREL:";
        snode::semantic::appLog().debug() << "================";
        printStandardHeader(pubrel);
        snode::semantic::appLog().debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubrel.getPacketIdentifier();

        if (pubrel.getPacketIdentifier() == 0) {
            snode::semantic::appLog().trace() << "PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            session->publishPacketIdentifierSet.erase(pubrel.getPacketIdentifier());

            sendPubcomp(pubrel.getPacketIdentifier());
        }

        onPubrel(pubrel);
    }

    void Mqtt::_onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        snode::semantic::appLog().debug() << "Received PUBCOMP:";
        snode::semantic::appLog().debug() << "=================";
        printStandardHeader(pubcomp);
        snode::semantic::appLog().debug() << "PacketIdentifier: 0x" << std::hex << std::setfill('0') << std::setw(4) << pubcomp.getPacketIdentifier();

        if (pubcomp.getPacketIdentifier() == 0) {
            snode::semantic::appLog().trace() << "PackageIdentifier missing";
            mqttContext->end(true);
        } else {
            session->publishMap.erase(pubcomp.getPacketIdentifier());
            session->pubrelPacketIdentifierSet.erase(pubcomp.getPacketIdentifier());
        }

        onPubcomp(pubcomp);
    }

    void Mqtt::printStandardHeader(const iot::mqtt::ControlPacket& packet) {
        snode::semantic::appLog().debug() << dataToHexString(packet.serialize());

        snode::semantic::appLog().debug() << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getType());
        snode::semantic::appLog().debug() << "Flags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getFlags());
        snode::semantic::appLog().debug() << "RemainingLength: " << std::dec << dynamic_cast<const ControlPacketDeserializer&>(packet).getRemainingLength();
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

    uint16_t Mqtt::getPacketIdentifier() {
        ++_packetIdentifier;

        if (_packetIdentifier == 0) {
            ++_packetIdentifier;
        }

        return _packetIdentifier;
    }

} // namespace iot::mqtt
