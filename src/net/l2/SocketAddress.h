/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
    private:
        using Super = net::SocketAddress<sockaddr_l2>;

    public:
        SocketAddress();
        explicit SocketAddress(const std::string& btAddress);
        explicit SocketAddress(uint16_t psm);
        SocketAddress(const std::string& btAddress, uint16_t psm);
        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen);

        SocketAddress& init();

        SocketAddress& setBtAddress(const std::string& btAddress);
        std::string getBtAddress() const;

        SocketAddress& setPsm(uint16_t psm);
        uint16_t getPsm() const;

        std::string toString(bool simple = false) const override;

    private:
        std::string btAddress;
        uint16_t psm = 0;
    };

} // namespace net::l2

extern template class net::SocketAddress<sockaddr_l2>;

#endif // NET_L2_SOCKETADDRESS_H
