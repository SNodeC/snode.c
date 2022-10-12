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

#ifndef IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBLISH_H
#define IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBLISH_H

#include "iot/mqtt/client/ControlPacketDeserializer.h" // IWYU pragma: export
#include "iot/mqtt/packets/Publish.h"                  // IWYU pragma: export

namespace iot::mqtt {
    class SocketContext;
    namespace client {
        class SocketContext;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client::packets {

    class Publish
        : public iot::mqtt::packets::Publish
        , public iot::mqtt::client::ControlPacketDeserializer {
    public:
        Publish(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::SocketContext* socketContext) override;
        void propagateEvent(iot::mqtt::client::SocketContext* socketContext) override;

        int state = 0;
    };

} // namespace iot::mqtt::client::packets

#endif // IOT_MQTT_CLIENT_PACKETS_DESERIALIZER_PUBLISH_H
