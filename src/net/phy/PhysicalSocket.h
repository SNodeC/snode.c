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

        int open(const std::map<int, const PhysicalSocketOption>& socketOptions, Flags flags);

        int bind(SocketAddress& bindAddress);

        bool isValid() const;

        int getSockError(int& cErrno) const;

        int getSockName(typename SocketAddress::SockAddr& localSockAddr, SocketAddress::SockLen& localSockAddrLen);
        int getPeerName(typename SocketAddress::SockAddr& remoteSockAddr, SocketAddress::SockLen& remoteSockAddrLen);
        /*
                template <typename ConfigT>
                SocketAddress getLocalSocketAddress(ConfigT& config) const;

                template <typename ConfigT>
                SocketAddress getRemoteSocketAddress(ConfigT& config) const;
        */
        SocketAddress getBindAddress() const;

    private:
        int setSockopt(int level, int optname, const void* optval, SocketAddress::SockLen optlen) const;
        int getSockopt(int level, int optname, void* optval, SocketAddress::SockLen* optlen) const;

    protected:
        SocketAddress bindAddress;

        int domain = 0;
        int type = 0;
        int protocol = 0;
    };

} // namespace net::phy

#endif // NET_PHY_SOCKET_H
