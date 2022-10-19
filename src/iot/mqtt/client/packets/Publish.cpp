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

#include "iot/mqtt/client/packets/Publish.h"

#include "iot/mqtt/client/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client::packets {

    Publish::Publish(uint32_t remainingLength, uint8_t flags)
        : iot::mqtt::client::ControlPacketDeserializer(remainingLength, flags, flags) {
        this->flags = flags;
        this->qoS = flags >> 1 & 0x03;
        this->dup = (flags & 0x04) != 0;
        this->retain = (flags & 0x01) != 0;

        error = this->qoS > 2;
    }

    std::size_t Publish::deserializeVP(iot::mqtt::SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0: // V-Header
                consumed += topic.deserialize(socketContext);
                if (!topic.isComplete()) {
                    break;
                }

                state++;
                [[fallthrough]];
            case 1:
                if (qoS > 0) {
                    consumed += packetIdentifier.deserialize(socketContext);
                    if (!packetIdentifier.isComplete()) {
                        break;
                    }
                }

                message.setSize(static_cast<uint16_t>(getRemainingLength() - getConsumed() - consumed));

                state++;
                [[fallthrough]];
            case 2: // Payload
                consumed += message.deserialize(socketContext);
                if (!message.isComplete()) {
                    break;
                }

                complete = true;
                break;
        }

        return consumed;
    }

    void Publish::propagateEvent(iot::mqtt::client::SocketContext* socketContext) {
        socketContext->_onPublish(*this);
    }

} // namespace iot::mqtt::client::packets
