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

#ifndef IOT_MQTT_MQTT_H
#define IOT_MQTT_MQTT_H

#include "core/timer/Timer.h"     // IWYU pragma: export
#include "iot/mqtt/FixedHeader.h" // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"

namespace core::socket {
    class SocketConnection;
}

namespace utils {
    class Timeval;
}

namespace iot::mqtt {
    class ControlPacketDeserializer;
    class MqttContext;

    namespace packets {
        class Pubrec;
        class Pubcomp;
    } // namespace packets
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <set>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class Mqtt {
    public:
        Mqtt() = default;

        virtual ~Mqtt();

        void setMqttContext(MqttContext* mqttContext);

        virtual void onConnected();
        std::size_t onReceiveFromPeer();
        virtual void onDisconnected();
        virtual void onExit();

        core::socket::SocketConnection* getSocketConnection();

    private:
        virtual iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& staticHeader) = 0;
        virtual void propagateEvent(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) = 0;

    public:
        void sendPublish(const std::string& topic, const std::string& message, uint8_t qoS, bool dup, bool retain);

    protected:
        void sendPuback(uint16_t packetIdentifier) const;
        void sendPubrec(uint16_t packetIdentifier) const;

        void sendPubrel(uint16_t packetIdentifier) const;
        void sendPubcomp(uint16_t packetIdentifier) const;

        void onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);

        static std::string dataToHexString(const std::vector<char>& data);

        uint16_t getPacketIdentifier();

        void send(iot::mqtt::ControlPacket&& controlPacket) const;
        void send(iot::mqtt::ControlPacket& controlPacket) const;
        void send(std::vector<char>&& data) const;

        static void printStandardHeader(const iot::mqtt::ControlPacket& packet);

        void setKeepAlive(const utils::Timeval& timeout);

    private:
        iot::mqtt::FixedHeader fixedHeader;
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        uint16_t packetIdentifier = 0;

        core::timer::Timer keepAliveTimer;

        int state = 0;

        std::set<uint16_t> packetIdentifierSet;
        std::map<uint16_t, iot::mqtt::packets::Publish> packetMap;

    protected:
        MqttContext* mqttContext = nullptr;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTT_H
