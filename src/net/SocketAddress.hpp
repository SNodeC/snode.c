/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/SocketAddress.h" // IWYU pragma: export

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
    const typename SocketAddress<SockAddr>::SockLen& SocketAddress<SockAddr>::getSockAddrLen() const {
        return sockAddrLen;
    }

    template <typename SockAddr>
    sa_family_t SocketAddress<SockAddr>::getAddressFamily() const {
        return reinterpret_cast<const sockaddr*>(&sockAddr)->sa_family;
    }

} // namespace net
