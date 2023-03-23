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

#include "core/socket/PhysicalSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h" // IWYU pragma: export

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddressT>
    class PhysicalSocket : public core::socket::PhysicalSocket<SocketAddressT> {
    private:
        using Super = core::socket::PhysicalSocket<SocketAddressT>;

    public:
        using SocketAddress = SocketAddressT;

        PhysicalSocket();
        PhysicalSocket(int fd);
        PhysicalSocket(int domain, int type, int protocol);

        PhysicalSocket& operator=(int fd) override;

        ~PhysicalSocket() override;

        int open(const std::map<int, const core::socket::PhysicalSocketOption>& socketOptions,
                 typename core::socket::PhysicalSocket<SocketAddress>::Flags flags) override;

        int bind(const SocketAddress& bindAddress) override;

        void shutdown(typename core::socket::PhysicalSocket<SocketAddress>::SHUT how) override;

        bool isValid() const override;

        int getSockError() const override;

        int getSockname(SocketAddress& socketAddress) override;
        int getPeername(SocketAddress& socketAddress) override;

        ssize_t sendFd(SocketAddress&& destAddress, int sendfd);
        ssize_t sendFd(SocketAddress& destAddress, int sendfd);
        ssize_t recvFd(int* recvfd);

        const SocketAddress& getBindAddress() const override;

    private:
        int setSockopt(int level, int optname, const void* optval, socklen_t optlen) const override;
        int getSockopt(int level, int optname, void* optval, socklen_t* optlen) const override;

        SocketAddress bindAddress{};

        int domain{};
        int type{};
        int protocol{};
    };

} // namespace net

#endif // NET_SOCKET_H
