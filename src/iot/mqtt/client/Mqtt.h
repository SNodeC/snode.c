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

#ifndef IOT_MQTT_CLIENT_SOCKETCONTEXT_H
#define IOT_MQTT_CLIENT_SOCKETCONTEXT_H

#include "iot/mqtt/Mqtt.h" // IWYU pragma: export
#include "iot/mqtt/client/packets/Publish.h"

// IWYU pragma: no_include "iot/mqtt/ControlPacketDeserializer.h"

namespace iot::mqtt {
    class Topic;

    namespace packets {
        class Connack;
        class Pingresp;
        class Puback;
        class Pubcomp;
        class Pubrec;
        class Pubrel;
        class Suback;
        class Unsuback;

    } // namespace packets

    namespace client::packets {
        class Connack;
        class Pingresp;
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
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    class Mqtt : public iot::mqtt::Mqtt {
    public:
        using Super = iot::mqtt::Mqtt;

        Mqtt() = default;

        ~Mqtt() override;

    private:
        iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) final;
        void propagateEvent(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) override;

        virtual void onConnack(const iot::mqtt::packets::Connack& connack);
        virtual void onPublish(const iot::mqtt::packets::Publish& publish);
        virtual void onPuback(const iot::mqtt::packets::Puback& puback);
        virtual void onPubrec(const iot::mqtt::packets::Pubrec& pubrec);
        virtual void onPubrel(const iot::mqtt::packets::Pubrel& pubrel);
        virtual void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp);
        virtual void onSuback(const iot::mqtt::packets::Suback& suback);
        virtual void onUnsuback(const iot::mqtt::packets::Unsuback& unsuback);
        virtual void onPingresp(const iot::mqtt::packets::Pingresp& pingresp);

        void _onConnack(const iot::mqtt::client::packets::Connack& connack);
        void _onPublish(const iot::mqtt::client::packets::Publish& publish);
        void _onPuback(const iot::mqtt::client::packets::Puback& puback);
        void _onPubrec(const iot::mqtt::client::packets::Pubrec& pubrec);
        void _onPubrel(const iot::mqtt::client::packets::Pubrel& pubrel);
        void _onPubcomp(const iot::mqtt::client::packets::Pubcomp& pubcomp);
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
                         const std::string& password);
        void sendSubscribe(std::list<Topic>& topics);
        void sendUnsubscribe(std::list<std::string>& topics);
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
        core::timer::Timer pingTimer;
    };

} // namespace iot::mqtt::client

#endif // IOT_MQTT_CLIENT_SOCKETCONTEXT_H
