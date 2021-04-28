/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_BLUETOOTH_ADDRESS_RFCOMMADDRESS_H
#define NET_SOCKET_BLUETOOTH_ADDRESS_RFCOMMADDRESS_H

#include "net/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h> // IWYU pragma: keep
#include <bluetooth/rfcomm.h>    // IWYU pragma: keep
#include <cstdint>               // for uint16_t
#include <string>
// IWYU pragma: no_include <bits/exception.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::bluetooth::address {

    class RfCommAddress : public SocketAddress<struct sockaddr_rc> {
    public:
        using SocketAddress<struct sockaddr_rc>::SocketAddress;

        explicit RfCommAddress(const std::string& btAddress);
        RfCommAddress(const std::string& btAddress, uint8_t channel);
        explicit RfCommAddress(uint8_t port);

        uint8_t channel() const;
        std::string address() const;

        std::string toString() const override;
    };

} // namespace net::socket::bluetooth::address

#endif // NET_SOCKET_BLUETOOTH_ADDRESS_RFCOMMADDRESS_H
