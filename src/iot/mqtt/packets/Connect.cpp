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

#include "iot/mqtt/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::packets {

    Connect::Connect(std::string clientId, std::string protocol, uint8_t version, uint8_t flags, uint16_t keepAlive)
        : iot::mqtt::ControlPacket(MQTT_CONNECT)
        , protocol(protocol)
        , version(version)
        , flags(flags)
        , keepAlive(keepAlive)
        , clientId(clientId) {
        putString(this->protocol);
        putInt8(this->version);
        putInt8(this->flags);
        putInt16(this->keepAlive);

        // Payload
        putString(this->clientId);
    }

    Connect::Connect(iot::mqtt::ControlPacketFactory& controlPacketFactory)
        : iot::mqtt::ControlPacket(controlPacketFactory) {
        protocol = getString();
        version = getInt8();
        flags = getInt8();
        keepAlive = getInt16();

        // Payload
        clientId = getString();

        error = isError();
    }

    std::string Connect::getProtocol() const {
        return protocol;
    }

    uint8_t Connect::getVersion() const {
        return version;
    }

    uint8_t Connect::getFlags() const {
        return flags;
    }

    uint16_t Connect::getKeepAlive() const {
        return keepAlive;
    }

    const std::string& Connect::getClientId() const {
        return clientId;
    }

} // namespace iot::mqtt::packets
