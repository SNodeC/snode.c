/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IOT_MQTT_MQTT_H
#define IOT_MQTT_MQTT_H

#include "core/timer/Timer.h"     // IWYU pragma: export
#include "iot/mqtt/FixedHeader.h" // IWYU pragma: export

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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    class Mqtt {
    public:
        Mqtt(const std::string& connectionName);
        Mqtt(const std::string& connectionName, const std::string& clientId);

        Mqtt(Mqtt&&) = delete;
        Mqtt(const Mqtt&) = delete;

        Mqtt& operator=(Mqtt&&) = delete;
        Mqtt& operator=(const Mqtt&) = delete;

        virtual ~Mqtt();

        virtual void onConnected();
        virtual void onDisconnected();
        virtual bool onSignal(int sig) = 0;

        const std::string& getConnectionName() const;

    private:
        std::size_t onReceivedFromPeer();
        void setMqttContext(MqttContext* mqttContext);

    public:
        const MqttContext* getMqttContext() const;

    private:
        virtual iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& staticHeader) = 0;
        virtual void deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) = 0;

    protected:
        void initSession(Session* session, utils::Timeval keepAlive);

    public:
        void sendPublish(const std::string& topic, const std::string& message, uint8_t qoS, bool retain);
        void sendPuback(uint16_t packetIdentifier) const;
        void sendPubrec(uint16_t packetIdentifier) const;
        void sendPubrel(uint16_t packetIdentifier) const;
        void sendPubcomp(uint16_t packetIdentifier) const;

    protected:
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

        uint16_t getPacketIdentifier();

        void send(const iot::mqtt::ControlPacket& controlPacket) const;

    public:
        static std::string toHexString(const std::vector<char>& data);
        static std::string toHexString(const std::string& data);

    private:
        void send(const std::vector<char>& data) const;

    protected:
        void printVP(const iot::mqtt::ControlPacket& packet) const;
        void printFixedHeader(const iot::mqtt::FixedHeader& fixedHeader) const;

        std::string connectionName;
        std::string clientId;

    private:
        iot::mqtt::FixedHeader fixedHeader;
        iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer = nullptr;

        uint16_t _packetIdentifier = 0;

        core::timer::Timer keepAliveTimer;

        int state = 0;

        Session* session = nullptr;

    protected:
        MqttContext* mqttContext = nullptr;

        friend class MqttContext;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTT_H
