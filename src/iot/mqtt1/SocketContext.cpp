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

#include "iot/mqtt1/SocketContext.h"

#include "iot/mqtt1/ControlPacket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1 {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection) {
    }

    SocketContext::~SocketContext() {
        if (currentPacket != nullptr) {
            delete currentPacket;
        }
    }

    std::size_t SocketContext::onReceiveFromPeer() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                // read static header
                consumed += staticHeader.construct(this);
                if (staticHeader.isError()) {
                    close();
                }
                if (staticHeader.isComplete()) {
                    switch (staticHeader.getPacketType()) {
                        case MQTT_CONNECT:
                            currentPacket = new iot::mqtt1::packets::Connect(staticHeader.getPacketType(), staticHeader.getPacketFlags());
                            break;
                    }

                    state++;
                }
                [[fallthrough]];
            case 1:
                LOG(TRACE) << "======================================================";
                LOG(TRACE) << "PacketType: " << static_cast<uint16_t>(staticHeader.getPacketType());
                LOG(TRACE) << "PacketFlags: " << static_cast<uint16_t>(staticHeader.getPacketFlags());
                LOG(TRACE) << "RemainingLength: " << static_cast<uint16_t>(staticHeader.getRemainingLength());

                consumed += currentPacket->construct(this);

                if (currentPacket->isComplete()) {
                    delete currentPacket;
                    currentPacket = nullptr;
                } else if (currentPacket->isError()) {
                    close();
                }

                break;
        }

        return consumed;
    }

    void SocketContext::printData(const std::vector<char>& data) const {
        std::stringstream ss;

        ss << "Data: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i + 1 != data.size()) {
                ss << std::endl;
                ss << "                                            ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }

        LOG(TRACE) << ss.str();
    }

} // namespace iot::mqtt1
