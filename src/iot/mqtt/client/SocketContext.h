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

#ifndef IOT_MQTT_SERVER_SOCKETCONTEXT_H
#define IOT_MQTT_SERVER_SOCKETCONTEXT_H

#include "iot/mqtt/SocketContext.h" // IWYU pragma: export
// IWYU pragma: no_include "iot/mqtt/ControlPacketDeserializer.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace iot::mqtt {
    class Topic;

    namespace packets {
        class Connack;
        class Pingresp;
        class Puback;
        class Pubcomp;
        class Publish;
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
        class Publish;
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
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    class SocketContext : public iot::mqtt::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    private:
        iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) final;
        void propagateEvent(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) override;

        virtual void onConnack(iot::mqtt::packets::Connack& connack);
        virtual void onPublish(iot::mqtt::packets::Publish& publish);
        virtual void onPuback(iot::mqtt::packets::Puback& puback);
        virtual void onPubrec(iot::mqtt::packets::Pubrec& pubrec);
        virtual void onPubrel(iot::mqtt::packets::Pubrel& pubrel);
        virtual void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp);
        virtual void onSuback(iot::mqtt::packets::Suback& suback);
        virtual void onUnsuback(iot::mqtt::packets::Unsuback& unsuback);
        virtual void onPingresp(iot::mqtt::packets::Pingresp& pingresp);

        void _onConnack(iot::mqtt::packets::Connack& connack);
        void _onPublish(iot::mqtt::packets::Publish& publish);
        void _onPuback(iot::mqtt::packets::Puback& puback);
        void _onPubrec(iot::mqtt::packets::Pubrec& pubrec);
        void _onPubrel(iot::mqtt::packets::Pubrel& pubrel);
        void _onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp);
        void _onSuback(iot::mqtt::packets::Suback& suback);
        void _onUnsuback(iot::mqtt::packets::Unsuback& unsuback);
        void _onPingresp(iot::mqtt::packets::Pingresp& pingresp);

    public:
        void sendConnect(uint16_t keepAlive,
                         const std::string& clientId,
                         bool cleanSession,
                         const std::string& willTopic,
                         const std::string& willMessage,
                         uint8_t willQoS,
                         bool willRetain,
                         const std::string& username,
                         const std::string& password) const;
        void sendSubscribe(uint16_t packetIdentifier, std::list<Topic>& topics) const;
        void sendUnsubscribe(uint16_t packetIdentifier, std::list<std::string>& topics) const;
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
    };

} // namespace iot::mqtt::client

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
