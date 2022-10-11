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

#include "iot/mqtt/packets/Connack.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connack::Connack()
        : iot::mqtt::ControlPacket(MQTT_CONNACK) {
    }

    Connack::Connack(uint8_t returncode, uint8_t flags)
        : iot::mqtt::ControlPacket(MQTT_CONNACK) {
        this->returnCode = returncode;
        this->connectFlags = flags;
    }

    std::vector<char> Connack::serializeVP() const {
        std::vector<char> packet;

        std::vector<char> tmpVector = connectFlags.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        tmpVector = returnCode.serialize();
        packet.insert(packet.end(), tmpVector.begin(), tmpVector.end());

        return packet;
    }

    uint8_t Connack::getFlags() const {
        return connectFlags;
    }

    uint8_t Connack::getReturnCode() const {
        return returnCode;
    }

    bool Connack::getSessionPresent() const {
        return sessionPresent;
    }

} // namespace iot::mqtt::packets
