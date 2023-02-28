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
    class PhysicalSocket : public core::socket::PhysicalSocket {
    private:
        using Super = core::socket::PhysicalSocket;

    public:
        using Super::Super;
        using Super::operator=;
        using SocketAddress = SocketAddressT;

    public:
        int bind(const SocketAddress& bindAddress);

        int getSockname(SocketAddress& socketAddress);
        int getPeername(SocketAddress& socketAddress);

        ssize_t sendFd(SocketAddress&& destAddress, int sendfd);
        ssize_t sendFd(SocketAddress& destAddress, int sendfd);
        ssize_t recvFd(int* recvfd);

        const SocketAddress& getBindAddress() const;

    private:
        SocketAddress bindAddress{};
    };

} // namespace net

#endif // NET_SOCKET_H
