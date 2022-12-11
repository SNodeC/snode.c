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

#ifndef IOT_MQTT_MQTT_H
#define IOT_MQTT_MQTT_H

#include "iot/mqtt/FixedHeader.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    class ControlPacketDeserializer;
    class ControlPacket;
    class MqttContext;
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class Mqtt {
    public:
        Mqtt() = default;
        Mqtt(const Mqtt&) = default;

        Mqtt& operator=(const Mqtt&) = default;

        virtual ~Mqtt();

        void setMqttContext(MqttContext* mqttContext);

        virtual void onConnected();
        std::size_t onReceiveFromPeer();
        virtual void onDisconnected();
        virtual void onExit();

    private:
        virtual iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& staticHeader) = 0;
        virtual void propagateEvent(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) = 0;

    public:
        void sendPublish(uint16_t packetIdentifier,
                         const std::string& topic,
                         const std::string& message,
                         uint8_t qoS = 0,
                         bool retain = false,
                         bool dup = false) const;
        void sendPuback(uint16_t packetIdentifier) const;
        void sendPubrec(uint16_t packetIdentifier) const;
        void sendPubrel(uint16_t packetIdentifier) const;
        void sendPubcomp(uint16_t packetIdentifier) const;

        static std::string dataToHexString(const std::vector<char>& data);

        uint16_t getPacketIdentifier();

        core::socket::SocketConnection* getSocketConnection();

    protected:
        void send(iot::mqtt::ControlPacket&& controlPacket) const;
        void send(iot::mqtt::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;

        static void printStandardHeader(const iot::mqtt::ControlPacket& packet);

    private:
        iot::mqtt::FixedHeader fixedHeader;
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        uint16_t packetIdentifier = 0;

        int state = 0;

    protected:
        MqttContext* mqttContext = nullptr;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTT_H