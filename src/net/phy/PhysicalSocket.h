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

#ifndef NET_PHY_SOCKET_H
#define NET_PHY_SOCKET_H

#include "core/Descriptor.h"              // IWYU pragma: export
#include "net/phy/PhysicalSocketOption.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h" // IWYU pragma: export

#include <functional>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::phy {

    template <typename SocketAddressT>
    class PhysicalSocket : public core::Descriptor {
    private:
        using Super = core::Descriptor;

    public:
        enum Flags { NONE = 0, NONBLOCK = SOCK_NONBLOCK, CLOEXIT = SOCK_CLOEXEC };

    protected:
        using SocketAddress = SocketAddressT;

        PhysicalSocket(int domain, int type, int protocol);
        PhysicalSocket(PhysicalSocket&& physicalSocket) noexcept = default;

        PhysicalSocket& operator=(PhysicalSocket&&) noexcept = default;

        ~PhysicalSocket() override;

    public:
        PhysicalSocket() = delete;
        PhysicalSocket(PhysicalSocket&) = delete;                  // only move is allowed for PhysicalSocket
        PhysicalSocket& operator=(PhysicalSocket&) = delete;       // only move is allowed for PhysicalSocket
        PhysicalSocket(const PhysicalSocket&) = delete;            // only move is allowed for PhysicalSocket
        PhysicalSocket& operator=(const PhysicalSocket&) = delete; // only move is allowed for PhysicalSocket

        PhysicalSocket(int fd, const SocketAddress& bindAddress);

        int open(const std::map<int, std::map<int, PhysicalSocketOption>>& socketOptionsMapMap, Flags flags);

        int bind(SocketAddress& bindAddress);

        bool isValid() const;

        int getSockError(int& cErrno) const;

        int getSockName(typename SocketAddress::SockAddr& localSockAddr, typename SocketAddress::SockLen& localSockAddrLen);
        int getPeerName(typename SocketAddress::SockAddr& remoteSockAddr, typename SocketAddress::SockLen& remoteSockAddrLen);

        SocketAddress getBindAddress() const;

    private:
        int setSockopt(int level, int optname, const void* optval, typename SocketAddress::SockLen optlen) const;
        int getSockopt(int level, int optname, void* optval, typename SocketAddress::SockLen* optlen) const;

    protected:
        SocketAddress bindAddress;

        int domain = 0;
        int type = 0;
        int protocol = 0;
    };

} // namespace net::phy

#endif // NET_PHY_SOCKET_H
