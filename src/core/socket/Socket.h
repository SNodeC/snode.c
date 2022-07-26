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

#ifndef CORE_SOCKET_SOCKET_H
#define CORE_SOCKET_SOCKET_H

#include "core/Descriptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    class Socket : public core::Descriptor {
    public:
        Socket();
        Socket(int fd); // cppcheck-suppress noExplicitConstructor
        Socket(int domain, int type, int protocol);

        Socket& operator=(int fd);

        virtual ~Socket() = default;

        enum Flags { NONE = 0, NONBLOCK = SOCK_NONBLOCK, CLOEXIT = SOCK_CLOEXEC };

    protected:
        int create(Flags flags) const;

        virtual void setSockopt();

    public:
        int open(Flags flags = Flags::NONE);

        bool isValid() const;

        int reuseAddress() const;
        int reusePort() const;

        int getSockError() const;

        int setSockopt(int level, int optname, const void* optval, socklen_t optlen) const;
        int getSockopt(int level, int optname, void* optval, socklen_t* optlen) const;

        enum SHUT { WR = SHUT_WR, RD = SHUT_RD, RDWR = SHUT_RDWR };

        void shutdown(SHUT how);

    protected:
        int domain{};
        int type{};
        int protocol{};
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKET_H
