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

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "core/Descriptor.h"
#include "core/socket/PhysicalSocketOption.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h" // IWYU pragma: export

#include <functional>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddressT>
    class PhysicalSocket : public core::Descriptor {
    private:
        using Super = core::Descriptor;

    public:
        using SocketAddress = SocketAddressT;

        PhysicalSocket();
        PhysicalSocket(int fd);
        PhysicalSocket(int domain, int type, int protocol);

        PhysicalSocket& operator=(int fd);

        ~PhysicalSocket() override;

        enum Flags { NONE = 0, NONBLOCK = SOCK_NONBLOCK, CLOEXIT = SOCK_CLOEXEC };

        int open(Flags flags);
        int open(const std::map<int, const core::socket::PhysicalSocketOption>& socketOptions, Flags flags);

        virtual int bind(const SocketAddress& bindAddress);

        enum SHUT { WR = SHUT_WR, RD = SHUT_RD, RDWR = SHUT_RDWR };

        void shutdown(SHUT how);

        bool isValid() const;

        int getSockError() const;

        int getSockname(SocketAddress& socketAddress);
        int getPeername(SocketAddress& socketAddress);

        ssize_t sendFd(SocketAddress&& destAddress, int sendfd);
        ssize_t sendFd(SocketAddress& destAddress, int sendfd);
        ssize_t recvFd(int* recvfd);

        const SocketAddress& getBindAddress() const;

    private:
        int setSockopt(int level, int optname, const void* optval, socklen_t optlen) const;
        int getSockopt(int level, int optname, void* optval, socklen_t* optlen) const;

        SocketAddress bindAddress{};

        int domain{};
        int type{};
        int protocol{};
    };

} // namespace net

#endif // NET_SOCKET_H
