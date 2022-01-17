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

#ifndef NET_RF_SOCKETADDRESS_H
#define NET_RF_SOCKETADDRESS_H

#include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h> // IWYU pragma: keep
#include <bluetooth/rfcomm.h>
#include <cstdint> // for uint16_t
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf {

    class SocketAddress : public core::socket::SocketAddress<struct sockaddr_rc> {
    public:
        using core::socket::SocketAddress<struct sockaddr_rc>::SocketAddress;

        SocketAddress();
        explicit SocketAddress(const std::string& btAddress);
        SocketAddress(const std::string& btAddress, uint8_t channel);
        explicit SocketAddress(uint8_t channel);

        uint8_t channel() const;

        std::string address() const override;
        std::string toString() const override;
    };

} // namespace net::rf

#endif // NET_RF_SOCKETADDRESS_H
