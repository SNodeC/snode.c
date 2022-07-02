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

#include "net/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SockAddrT>
    SocketAddress<SockAddrT>::SocketAddress(socklen_t addrLen)
        : sockAddr{}
        , addrLen(addrLen) {
    }

    template <typename SockAddrT>
    SocketAddress<SockAddrT>::SocketAddress(const SocketAddress& socketAddress) {
        *this = socketAddress;
    }

    template <typename SockAddrT>
    SocketAddress<SockAddrT>& SocketAddress<SockAddrT>::operator=(const SocketAddress& socketAddress) {
        if (this != &socketAddress) {
            this->sockAddr = socketAddress.sockAddr;
            this->addrLen = socketAddress.addrLen;
        }

        return *this;
    }

    template <typename SockAddrT>
    SocketAddress<SockAddrT>::operator sockaddr*() {
        return reinterpret_cast<sockaddr*>(&sockAddr);
    }

    template <typename SockAddrT>
    SocketAddress<SockAddrT>::operator const sockaddr*() const {
        return reinterpret_cast<const sockaddr*>(&sockAddr);
    }

    template <typename SockAddrT>
    sockaddr& SocketAddress<SockAddrT>::getSockAddr() {
        return reinterpret_cast<sockaddr&>(sockAddr);
    }

    template <typename SockAddrT>
    const sockaddr& SocketAddress<SockAddrT>::getSockAddr() const {
        return reinterpret_cast<const sockaddr&>(sockAddr);
    }

    template <typename SockAddrT>
    socklen_t& SocketAddress<SockAddrT>::getAddrLen() {
        return addrLen;
    }

    template <typename SockAddrT>
    const socklen_t& SocketAddress<SockAddrT>::getAddrLen() const {
        return addrLen;
    }

} // namespace net
