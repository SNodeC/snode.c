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

#ifndef MQTT_SOCKETCONTEXT_H
#define MQTT_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
#include "mqtt/ControlPacketFactory.h"

namespace core::socket {
    class SocketConnection;
}

namespace mqtt::packets {
    class Connect;
    class Publish;
    class Subscribe;
} // namespace mqtt::packets

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    protected:
        virtual void onControlPackageReceived(std::vector<char>& controlPacket) = 0;

        virtual void onConnect(const mqtt::packets::Connect& connect);
        virtual void onPublish(const mqtt::packets::Publish& publish);
        virtual void onSubscribe(const mqtt::packets::Subscribe& subscribe);

        // virtual void onConnack(const mqtt::packets::Connack& connack);
        // virtual void onPublish(const mqtt::packets::Publish& publish);
        // virtual void onPuback(const mqtt::packets::Puback& puback);
        // virtual void onPubrec(const mqtt::packets::Pubrec& pubrec);
        // virtual void onPubrel(const mqtt::packets::Pubrel& pubrel);
        // virtual void onPubcomp(const mqtt::packets::Pubcomp& pubcomp);
        // virtual void onSubscribe(const mqtt::packets::Subscribe& subscribe);
        // virtual void onSuback(const mqtt::packets::Suback& suback);
        // virtual void onUnsubscribe(const mqtt::packets::Unsubscribe& unsubscribe);
        // virtual void onUnsuback(const mqtt::packets::Unsuback& unsuback);
        // virtual void onPingreq(const mqtt::packets::Pingreq& pingreq);
        // virtual void onPingresp(const mqtt::packets::Pingresp& pingresp);
        // virtual void onDisconnect(const mqtt::packets::Disconnect& disconnect);
        // virtual void onAuth(const mqtt::packets::Auth& auth);

        void sendConnack();
        void sendPuback(uint16_t packetIdentifier);
        void sendSuback(uint16_t packetIdentifier, std::list<uint8_t>& returnCodes);

    private:
        virtual std::size_t onReceiveFromPeer() final;

        void printData(std::vector<char>& data);
        void send(std::vector<char>& data);

        ControlPacketFactory controlPacketFactory;
    };

} // namespace mqtt

#endif // MQTT_SOCKETCONTEXT_H
