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

namespace core::socket::stream {
    class SocketConnection;
}

namespace iot::mqtt {
    class ControlPacket;
    class ControlPacketDeserializer;
    class MqttContext;
    class Session;

    namespace packets {
        class Publish;
        class Puback;
        class Pubrec;
        class Pubrel;
        class Pubcomp;
    } // namespace packets
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    class Mqtt {
    public:
        Mqtt() = default;

        virtual ~Mqtt();

        void setMqttContext(MqttContext* mqttContext);

        virtual void onConnected();
        std::size_t onProcess();
        virtual void onDisconnected();
        virtual void onExit();

        core::socket::stream::SocketConnection* getSocketConnection();

    private:
        virtual iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& staticHeader) = 0;
        virtual void deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) = 0;

    protected:
        void initSession(Session* session, const utils::Timeval& keepAlive);

    public:
        void sendPublish(const std::string& topic, const std::string& message, uint8_t qoS, bool retain);

    protected:
        void sendPuback(uint16_t packetIdentifier) const;
        void sendPubrec(uint16_t packetIdentifier) const;
        void sendPubrel(uint16_t packetIdentifier) const;
        void sendPubcomp(uint16_t packetIdentifier) const;

        virtual void onPublish(const iot::mqtt::packets::Publish& publish);
        virtual void onPuback(const iot::mqtt::packets::Puback& puback);
        virtual void onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        virtual void onPubrel(const iot::mqtt::packets::Pubrel& pubrel);
        virtual void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);

        bool _onPublish(const iot::mqtt::packets::Publish& publish);
        void _onPuback(const iot::mqtt::packets::Puback& puback);
        void _onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        void _onPubrel(const iot::mqtt::packets::Pubrel& pubrel);
        void _onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);

        static std::string dataToHexString(const std::vector<char>& data);

        uint16_t getPacketIdentifier();

        void send(const iot::mqtt::ControlPacket& controlPacket) const;
        void send(const std::vector<char>& data) const;

        static void printStandardHeader(const iot::mqtt::ControlPacket& packet);

    private:
        iot::mqtt::FixedHeader fixedHeader;
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        uint16_t _packetIdentifier = 0;

        core::timer::Timer keepAliveTimer;

        int state = 0;

        Session* session = nullptr;

    protected:
        MqttContext* mqttContext = nullptr;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTT_H
