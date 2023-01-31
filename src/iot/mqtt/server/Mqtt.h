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

#ifndef IOT_MQTT_SERVER_SOCKETCONTEXT_H
#define IOT_MQTT_SERVER_SOCKETCONTEXT_H

#include "iot/mqtt/Mqtt.h" // IWYU pragma: export

// IWYU pragma: no_include "iot/mqtt/ControlPacketDeserializer.h"

namespace iot::mqtt {
    namespace packets {
        class Connect;
        class Disconnect;
        class Pingreq;
        class Subscribe;
        class Unsubscribe;

    } // namespace packets

    namespace server {
        namespace broker {
            class Broker;
        }

        namespace packets {
            class Connect;
            class Disconnect;
            class Pingreq;
            class Publish;
            class Puback;
            class Pubcomp;
            class Pubrec;
            class Pubrel;
            class Subscribe;
            class Unsubscribe;
        } // namespace packets
    }     // namespace server
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server {

    class Mqtt : public iot::mqtt::Mqtt {
    public:
        using Super = iot::mqtt::Mqtt;

        explicit Mqtt(const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker);
        ~Mqtt() override;

    private:
        iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader& fixedHeader) final;
        void deliverPacket(iot::mqtt::ControlPacketDeserializer* controlPacketDeserializer) override;

        void onExit() override;

        using Super::initSession;
        void initSession(const utils::Timeval& keepAlive);
        void releaseSession();

        virtual void onConnect(const iot::mqtt::packets::Connect& connect);
        virtual void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe);
        virtual void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe);
        virtual void onPingreq(const iot::mqtt::packets::Pingreq& pingreq);
        virtual void onDisconnect(const iot::mqtt::packets::Disconnect& disconnect);

        void _onConnect(const iot::mqtt::server::packets::Connect& connect);
        void _onPublish(const iot::mqtt::server::packets::Publish& publish);
        void _onSubscribe(const iot::mqtt::server::packets::Subscribe& subscribe);
        void _onUnsubscribe(const iot::mqtt::server::packets::Unsubscribe& unsubscribe);
        void _onPingreq(const iot::mqtt::server::packets::Pingreq& pingreq);
        void _onDisconnect(const iot::mqtt::server::packets::Disconnect& disconnect);

    public:
        void sendConnack(uint8_t returnCode, uint8_t flags) const;
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes) const;
        void sendUnsuback(uint16_t packetIdentifier) const;
        void sendPingresp() const;

        std::string getProtocol() const;
        uint8_t getLevel() const;
        uint8_t getConnectFlags() const;
        uint16_t getKeepAlive() const;
        std::string getClientId() const;
        std::string getWillTopic() const;
        std::string getWillMessage() const;
        std::string getUsername() const;
        std::string getPassword() const;
        bool getUsernameFlag() const;
        bool getPasswordFlag() const;
        bool getWillRetain() const;
        uint8_t getWillQoS() const;
        bool getWillFlag() const;
        bool getCleanSession() const;

    protected:
        std::string protocol;
        uint8_t level = 0;
        uint8_t connectFlags = 0;
        uint16_t keepAlive = 0;

        std::string clientId;
        std::string willTopic;
        std::string willMessage;
        std::string username;
        std::string password;

        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;

        std::shared_ptr<iot::mqtt::server::broker::Broker> broker;

        friend class iot::mqtt::server::packets::Connect;
        friend class iot::mqtt::server::packets::Subscribe;
        friend class iot::mqtt::server::packets::Unsubscribe;
        friend class iot::mqtt::server::packets::Pingreq;
        friend class iot::mqtt::server::packets::Disconnect;
        friend class iot::mqtt::server::packets::Publish;
        friend class iot::mqtt::server::packets::Pubcomp;
        friend class iot::mqtt::server::packets::Pubrec;
        friend class iot::mqtt::server::packets::Puback;
        friend class iot::mqtt::server::packets::Pubrel;
    };

} // namespace iot::mqtt::server

#endif // IOT_MQTT_SERVER_SOCKETCONTEXT_H
