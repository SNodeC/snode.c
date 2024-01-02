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

#include "net/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SockAddr>
    SocketAddress<SockAddr>::SocketAddress(sa_family_t af, SockLen sockAddrLen)
        : sockAddrLen(sockAddrLen) {
        std::memset(&sockAddr, 0, sizeof(sockAddr));
        reinterpret_cast<sockaddr*>(&sockAddr)->sa_family = af;
    }

    template <typename SockAddr>
    SocketAddress<SockAddr>::SocketAddress(const SocketAddress& socketAddress)
        : sockAddr(socketAddress.sockAddr)
        , sockAddrLen(socketAddress.sockAddrLen) {
    }

    template <typename SockAddr>
    SocketAddress<SockAddr>::SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen)
        : sockAddr(sockAddr)
        , sockAddrLen(sockAddrLen) {
    }

    template <typename SockAddr>
    SocketAddress<SockAddr>& SocketAddress<SockAddr>::operator=(const SocketAddress& socketAddress) {
        if (this != &socketAddress) {
            this->sockAddr = socketAddress.sockAddr;
            this->sockAddrLen = socketAddress.sockAddrLen;
        }

        return *this;
    }

    template <typename SockAddr>
    const sockaddr& SocketAddress<SockAddr>::getSockAddr() {
        return reinterpret_cast<const sockaddr&>(sockAddr);
    }

    template <typename SockAddr>
    const SocketAddress<SockAddr>::SockLen& SocketAddress<SockAddr>::getSockAddrLen() const {
        return sockAddrLen;
    }

    template <typename SockAddr>
    sa_family_t SocketAddress<SockAddr>::getAddressFamily() const {
        return reinterpret_cast<const sockaddr*>(&sockAddr)->sa_family;
    }

} // namespace net
