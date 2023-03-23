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

#ifndef CORE_SOCKET_SOCKET_H
#define CORE_SOCKET_SOCKET_H

#include "core/Descriptor.h"

namespace core::socket {
    class PhysicalSocketOption;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    template <typename SocketAddressT>
    class PhysicalSocket : public core::Descriptor {
    public:
        using Super = core::Descriptor;
        using SocketAddress = SocketAddressT;

        virtual PhysicalSocket& operator=(int fd) = 0;

        enum Flags { NONE = 0, NONBLOCK = SOCK_NONBLOCK, CLOEXIT = SOCK_CLOEXEC };

    public:
        virtual int open(const std::map<int, const PhysicalSocketOption>& socketOptions, Flags flags = Flags::NONE) = 0;

        virtual int bind(const SocketAddress& bindAddress) = 0;

        virtual int getSockname(SocketAddress& socketAddress) = 0;
        virtual int getPeername(SocketAddress& socketAddress) = 0;

        virtual bool isValid() const = 0;

        virtual int getSockError() const = 0;

        enum SHUT { WR = SHUT_WR, RD = SHUT_RD, RDWR = SHUT_RDWR };

        virtual void shutdown(SHUT how) = 0;

        virtual const SocketAddress& getBindAddress() const = 0;

    private:
        virtual int setSockopt(int level, int optname, const void* optval, socklen_t optlen) const = 0;
        virtual int getSockopt(int level, int optname, void* optval, socklen_t* optlen) const = 0;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKET_H
