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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h> // IWYU pragma: keep, for str2ba, ba2str, bdaddr_t
#include <bluetooth/rfcomm.h>    // IWYU pragma: keep, for sockaddr_rc
#include <cstdint>               // for uint16_t
#include <exception>             // IWYU pragma: keep
#include <string>
// IWYU pragma: no_include <bits/exception.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketAddress.h"

namespace net::socket::bluetooth::address {

    class RfCommAddress : public SocketAddress<struct sockaddr_rc> {
    public:
        RfCommAddress();
        RfCommAddress(const RfCommAddress& bta);

        explicit RfCommAddress(const std::string& btAddress);
        RfCommAddress(const std::string& btAddress, uint8_t channel);
        explicit RfCommAddress(uint8_t port);
        explicit RfCommAddress(const struct sockaddr_rc& addr);

        uint8_t channel() const;
        std::string address() const;

        std::string toString() const override;

        RfCommAddress& operator=(const RfCommAddress& bta);

        const struct sockaddr_rc& getSockAddrRc() const;
    };

} // namespace net::socket::bluetooth::address

#endif // NET_SOCKET_BLUETOOTH_ADDRESS_RFCOMMADDRESS_H
