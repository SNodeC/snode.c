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

#ifndef IOT_MQTT_SOCKETCONTEXT_H
#define IOT_MQTT_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export
#include "iot/mqtt/StaticHeader.h"     // IWYU pragma: export
#include "iot/mqtt/packets/Puback.h"   // IWYU pragma: export
#include "iot/mqtt/packets/Pubcomp.h"  // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"  // IWYU pragma: export
#include "iot/mqtt/packets/Pubrec.h"   // IWYU pragma: export
#include "iot/mqtt/packets/Pubrel.h"   // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class SocketContext : public core::socket::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        virtual ~SocketContext() override;

    private:
        std::size_t onReceiveFromPeer() final;
        virtual iot::mqtt::ControlPacketReceiver* deserialize(iot::mqtt::StaticHeader& staticHeader) = 0;

        virtual void onPublish(iot::mqtt::packets::Publish& publish) = 0;
        virtual void onPuback(iot::mqtt::packets::Puback& puback) = 0;
        virtual void onPubrec(iot::mqtt::packets::Pubrec& pubrec) = 0;
        virtual void onPubrel(iot::mqtt::packets::Pubrel& pubrel) = 0;
        virtual void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) = 0;

        void _onPublish(iot::mqtt::packets::Publish& publish);
        void _onPuback(iot::mqtt::packets::Puback& puback);
        void _onPubrec(iot::mqtt::packets::Pubrec& pubrec);
        void _onPubrel(iot::mqtt::packets::Pubrel& pubrel);
        void _onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp);

    public:
        void sendPublish(const std::string& topic, const std::string& message, bool dup = false, uint8_t qoSLevel = 0, bool retain = false);
        void sendPuback(uint16_t packetIdentifier);
        void sendPubrec(uint16_t packetIdentifier);
        void sendPubrel(uint16_t packetIdentifier);
        void sendPubcomp(uint16_t packetIdentifier);

    protected:
        void send(iot::mqtt::ControlPacketSender&& controlPacket) const;
        void send(iot::mqtt::ControlPacketSender& controlPacket) const;
        void send(std::vector<char>&& data) const;

        std::string getRandomClientId();

        uint16_t getPacketIdentifier();

    private:
        static void printData(const std::vector<char>& data);

        iot::mqtt::StaticHeader staticHeader;
        iot::mqtt::ControlPacketReceiver* currentPacket = nullptr;

        uint16_t packetIdentifier = 0;

        int state = 0;

        friend class iot::mqtt::packets::Publish;
        friend class iot::mqtt::packets::Pubcomp;
        friend class iot::mqtt::packets::Pubrec;
        friend class iot::mqtt::packets::Puback;
        friend class iot::mqtt::packets::Pubrel;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_SOCKETCONTEXT_H
