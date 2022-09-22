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
#include <endian.h>
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
        VLOG(0) << "Flags: " << static_cast<uint16_t>(connect.flags());
        VLOG(0) << "Protocol: " << connect.protocol();
        VLOG(0) << "Version: " << static_cast<uint16_t>(connect.version());

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
        VLOG(0) << "Topic: " << publish.getName();
        VLOG(0) << "Message: " << publish.getMessage();
        VLOG(0) << "PacketIdentifier: " << publish.getPacketIdentifier();

        //        sendPublishToAll(publish.getData());

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

        sendPublish(subscribe.getPacketIdentifier(), "hihi/hoho", "hallo");
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

        std::vector<char> data;

        data.push_back(0x00);
        data.push_back(0x04);
        data.push_back('M');
        data.push_back('Q');
        data.push_back('T');
        data.push_back('T');

        data.push_back(0x04); // Protocol Level (4)

        data.push_back(0x02); // Connect Flags (Clean Session)

        data.push_back(0x00); // Keep Alive MSB (60Sec)
        data.push_back(0x3C); // Keep Alive LSB

        // Payload
        data.push_back(0x00); // Client ID Length MSB (2)
        data.push_back(0x02); // Client ID Length LSB

        data.push_back('C');
        data.push_back('L');

        // Fill Packet
        std::vector<char> packet;
        packet.push_back(0x10); // Connect Type: 0x01, Flags: 0x00

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendConnack() {
        // Data: 0x20 0x03 0x00 0x00 0x00
        VLOG(0) << "Send CONNACK";
        VLOG(0) << "============";

        std::vector<char> data;

        data.push_back(0x00); // Connack Flags: 0x00 - LSB is session present
        data.push_back(0x00); // Connack Reason: 0x00 = Success
        // data.push_back(0x00); // (only in V5) Connack Property Length: 0x00 - no properties

        std::vector<char> packet;
        packet.push_back(0x20); // Connack Type: 0x02, Flags: 0x00

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendPublish(
        uint16_t packetIdentifier, const std::string& topic, const std::string& message, bool dup, uint8_t qos, bool retain) {
        VLOG(0) << "Send PUBLISH";
        VLOG(0) << "============";

        std::vector<char> data;

        uint16_t topicLen = static_cast<uint16_t>(topic.size());
        uint16_t topicLenBE = htobe16(topicLen);
        data.push_back(static_cast<char>(topicLenBE & 0xFF));
        data.push_back(static_cast<char>(topicLenBE >> 0x08));

        for (char ch : topic) {
            data.push_back(ch);
        }

        if (qos > 0) {
            uint16_t packetIdentifierBE = htobe16(packetIdentifier);
            data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
            data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));
        }

        for (char ch : message) {
            data.push_back(ch);
        }

        // Fill Packet
        std::vector<char> packet;
        packet.push_back(0x30 | (dup ? 0x04 : 0x00) | (qos ? (qos << 1) & 0x03 : 0x00) |
                         (retain ? 0x01 : 0x00)); // Publish Type: 0x03, Flags: 0x00 (DUP(1), QoS(2), RETAIN(1))

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendPuback(uint16_t packetIdentifier) {
        VLOG(0) << "Send PUBACK";
        VLOG(0) << "===========";

        std::vector<char> data;

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        std::vector<char> packet;
        packet.push_back(0x40); // Puback Type: 0x04

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendSubscribe(uint16_t packetIdentifier, std::list<std::string>& topics, uint8_t qos) {
        VLOG(0) << "Send SUBSCRIBE";
        VLOG(0) << "==============";

        std::vector<char> data;

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        for (std::string& topic : topics) {
            uint16_t topicLen = static_cast<uint16_t>(topic.size());

            uint16_t topicLenBE = htobe16(topicLen);
            data.push_back(static_cast<char>(topicLenBE & 0xFF));
            data.push_back(static_cast<char>(topicLenBE >> 0x08));

            for (char ch : topic) {
                data.push_back(ch);
            }
            data.push_back(static_cast<char>(qos)); // Topic QoS
        }

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0x80 | 0x02)); // Subscribe Type: 0x08, Reserved: 0x02

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) {
        VLOG(0) << "Send SUBACK";
        VLOG(0) << "===========";

        std::vector<char> data;

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        for (uint8_t returnCode : returnCodes) {
            data.push_back(static_cast<char>(returnCode));
        }

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0x90)); // Suback Type: 0x09

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendUnsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics) {
        VLOG(0) << "Send UNSUBSCRIBE";
        VLOG(0) << "================";

        std::vector<char> data;

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        for (std::string& topic : topics) {
            uint16_t topicLen = static_cast<uint16_t>(topic.size());

            uint16_t topicLenBE = htobe16(topicLen);
            data.push_back(static_cast<char>(topicLenBE & 0xFF));
            data.push_back(static_cast<char>(topicLenBE >> 0x08));

            for (char ch : topic) {
                data.push_back(ch);
            }
        }

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0xA0 | 0x02)); // Unsubscribe Type: 0x0A, Reserved: 0x02

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendUnsuback(uint16_t packetIdentifier) {
        VLOG(0) << "Send UNSUBACK";
        VLOG(0) << "=============";

        std::vector<char> data;

        uint16_t packetIdentifierBE = htobe16(packetIdentifier);
        data.push_back(static_cast<char>(packetIdentifierBE & 0xFF));
        data.push_back(static_cast<char>(packetIdentifierBE >> 0x08));

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0xB0)); // Unsuback Type: 0x0B

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendPingreq() {
        VLOG(0) << "Send Pingreq";
        VLOG(0) << "============";

        std::vector<char> data;

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0xC0)); // Pingreq Type: 0x0C

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
    }

    void SocketContext::sendPingresp() {
        VLOG(0) << "Send Pingresp";
        VLOG(0) << "=============";

        std::vector<char> data;

        std::vector<char> packet;
        packet.push_back(static_cast<char>(0xD0)); // Pingresp Type: 0x0D

        uint64_t remainingLength = data.size();
        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingLength % 0x80);
            remainingLength /= 0x80;
            if (remainingLength > 0) {
                encodedByte |= 0x80;
            }
            packet.push_back(static_cast<char>(encodedByte));
        } while (remainingLength > 0);

        packet.insert(packet.end(), data.begin(), data.end());

        send(packet);
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
                case 0x01:
                    onConnect(mqtt::packets::Connect(controlPacketFactory));
                    break;
                case 0x02:
                    onConnack(mqtt::packets::Connack(controlPacketFactory));
                    break;
                case 0x03:
                    onPublish(mqtt::packets::Publish(controlPacketFactory));
                    break;
                case 0x04:
                    onPuback(mqtt::packets::Puback(controlPacketFactory));
                    break;
                case 0x05:
                    // onPubrec(Pubrec(controlPacketFactory.get()));
                    break;
                case 0x06:
                    // onPubrel(Pubrel(controlPacketFactory.get()));
                    break;
                case 0x07:
                    // onPubcomp(Pubcomp(controlPacketFactory.get()));
                    break;
                case 0x08:
                    onSubscribe(mqtt::packets::Subscribe(controlPacketFactory));
                    break;
                case 0x09:
                    onSuback(mqtt::packets::Suback(controlPacketFactory));
                    break;
                case 0x0A:
                    onUnsubscribe(mqtt::packets::Unsubscribe(controlPacketFactory));
                    break;
                case 0x0B:
                    onUnsuback(mqtt::packets::Unsuback(controlPacketFactory));
                    break;
                case 0x0C:
                    onPingreq(mqtt::packets::Pingreq(controlPacketFactory));
                    break;
                case 0x0D:
                    onPingresp(mqtt::packets::Pingresp(controlPacketFactory));
                    break;
                case 0x0E:
                    onDisconnect(mqtt::packets::Disconnect(controlPacketFactory));
                    break;
                case 0x0F:
                    // onAuth(Auth(controlPacketFactory.get()));
                    break;
                default:
                    // onUnknown(Unknown(controlPacketFactory.get()));
                    break;
            }

            onControlPackageReceived(controlPacketFactory.getPacket());

            controlPacketFactory.reset();
        }

        return consumed;
    }

    void SocketContext::printData(std::vector<char>& data) {
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

    void SocketContext::send(std::vector<char>& data) {
        printData(data);
        sendToPeer(data.data(), data.size());
    }

} // namespace iot::mqtt
