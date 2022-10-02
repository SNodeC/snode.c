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

#include "iot/mqtt1/ControlPacket.h"

#include "iot/mqtt1/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    ControlPacket::ControlPacket(uint8_t type, uint8_t reserved)
        : staticHeader(type, reserved) {
    }

    ControlPacket::~ControlPacket() {
        if (currentPacket != nullptr) {
            delete currentPacket;
        }
    }

    std::size_t ControlPacket::construct(SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                // read static header
                consumed += staticHeader.construct(socketContext);
                if (staticHeader.isError()) {
                    socketContext->close();
                }
                if (staticHeader.isComplete()) {
                    // Create concrete packet
                    switch (staticHeader.getPacketType()) {
                        case MQTT_CONNECT:
                            currentPacket = new iot::mqtt1::packets::Connect(staticHeader.getPacketType(), staticHeader.getReserved());
                            break;
                    }

                    state++;
                }
                [[fallthrough]];
            case 1:
                // read concretes packet variable header and payload
                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "PacketType: " << static_cast<uint16_t>(staticHeader.getPacketType());
                LOG(TRACE) << "Reserved: " << static_cast<uint16_t>(staticHeader.getReserved());
                LOG(TRACE) << "RemainingLength: " << static_cast<uint16_t>(staticHeader.getRemainingLength());

                consumed += currentPacket->construct(socketContext);

                if (currentPacket->isComplete()) {
                    delete currentPacket;
                    currentPacket = nullptr;
                } else if (currentPacket->isError()) {
                    socketContext->close();
                }

                break;
        }

        this->consumed += consumed;

        return consumed;
    }

    uint8_t ControlPacket::getType() const {
        return staticHeader.getPacketType();
    }

    uint8_t ControlPacket::getReserved() const {
        return staticHeader.getReserved();
    }

    uint32_t ControlPacket::getRemainingLength() const {
        return staticHeader.getRemainingLength();
    }

    bool ControlPacket::isComplete() const {
        return complete;
    }

    bool ControlPacket::isError() const {
        return error;
    }

    uint64_t ControlPacket::getConsumed() const {
        return consumed;
    }

    std::vector<char> ControlPacket::getPacket() const {
        std::vector<char> packet = currentPacket->getPacket();

        iot::mqtt1::types::StaticHeader staticHeader(currentPacket->getType(), currentPacket->getReserved());
        staticHeader.setRemainingLength(static_cast<uint32_t>(packet.size()));

        std::vector<char> packetStaticHeader = staticHeader.getPacket();

        packetStaticHeader.insert(packetStaticHeader.end(), packet.begin(), packet.end());

        return packetStaticHeader;
    }

} // namespace iot::mqtt1
