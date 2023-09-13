/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef NET_L2_SOCKETADDRESS_H
#define NET_L2_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h> // IWYU pragma: keep
#include <bluetooth/l2cap.h>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2 {

    class SocketAddress final : public net::SocketAddress<sockaddr_l2> {
    public:
        using net::SocketAddress<sockaddr_l2>::SocketAddress;

        SocketAddress();
        explicit SocketAddress(const std::string& btAddress);
        SocketAddress(const std::string& btAddress, uint16_t psm);
        explicit SocketAddress(uint16_t psm);

        SocketAddress setBtAddress(const std::string& btAddress);
        SocketAddress setPsm(uint16_t psm);

        uint16_t psm() const;

        std::string address() const override;
        std::string toString() const override;
    };

} // namespace net::l2

#endif // NET_L2_SOCKETADDRESS_H
