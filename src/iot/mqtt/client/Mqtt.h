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

#ifndef IOT_MQTT_CLIENT_SOCKETCONTEXT_H
#define IOT_MQTT_CLIENT_SOCKETCONTEXT_H

#include "iot/mqtt/Mqtt.h" // IWYU pragma: export
#include "iot/mqtt/Session.h"

// IWYU pra gma: no_include "iot/mqtt/ControlPacketDeserializer.h"

namespace iot::mqtt {
    class Topic;

    namespace packets {
        class Connack;
        class Pingresp;
        class Suback;
        class Unsuback;

    } // namespace packets

    namespace client::packets {
        class Connack;
        class Pingresp;
        class Publish;
        class Puback;
        class Pubcomp;
        class Pubrec;
        class Pubrel;
        class Suback;
        class Unsuback;
    } // namespace client::packets
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::client {

    class Mqtt : public iot::mqtt::Mqtt {
    public:
        using Super = iot::mqtt::Mqtt;

        explicit Mqtt(const std::string& connectionName, const std::string& clientId, const std::string& sessionStoreFileName);

        ~Mqtt() override;

    private:
        iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) final;
        void deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) override;

    protected:
        bool onSignal(int sig) override;

    private:
        virtual void onConnack(const iot::mqtt::packets::Connack& connack);
        virtual void onSuback(const iot::mqtt::packets::Suback& suback);
        virtual void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback);
        virtual void onPingresp(const iot::mqtt::packets::Pingresp& pingresp);

        void _onConnack(const iot::mqtt::client::packets::Connack& connack);
        void _onPublish(const iot::mqtt::client::packets::Publish& publish);
        void _onSuback(const iot::mqtt::client::packets::Suback& suback);
        void _onUnsuback(const iot::mqtt::client::packets::Unsuback& unsuback);
        void _onPingresp(const iot::mqtt::client::packets::Pingresp& pingresp);

    public:
        void sendConnect(uint16_t keepAlive,
                         const std::string& clientId,
                         bool cleanSession,
                         const std::string& willTopic,
                         const std::string& willMessage,
                         uint8_t willQoS,
                         bool willRetain,
                         const std::string& username,
                         const std::string& password,
                         bool loopPrevention = false);
        void sendSubscribe(const std::list<Topic>& topics);
        void sendUnsubscribe(const std::list<std::string>& topics);
        void sendPingreq() const;
        void sendDisconnect() const;

        friend class iot::mqtt::client::packets::Connack;
        friend class iot::mqtt::client::packets::Suback;
        friend class iot::mqtt::client::packets::Unsuback;
        friend class iot::mqtt::client::packets::Pingresp;
        friend class iot::mqtt::client::packets::Publish;
        friend class iot::mqtt::client::packets::Pubcomp;
        friend class iot::mqtt::client::packets::Pubrec;
        friend class iot::mqtt::client::packets::Puback;
        friend class iot::mqtt::client::packets::Pubrel;

    private:
        uint16_t keepAlive = 0;

        std::string sessionStoreFileName;
        iot::mqtt::Session session;

        core::timer::Timer pingTimer;
    };

} // namespace iot::mqtt::client

#endif // IOT_MQTT_CLIENT_SOCKETCONTEXT_H
