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

#include "core/socket/Socket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    Socket::Socket() {
        Descriptor::open(-1);
    }

    Socket::Socket(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);
    }

    Socket::Socket(int domain, int type, int protocol)
        : domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    Socket& Socket::operator=(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);

        return *this;
    }

    int Socket::create(Flags flags) const {
        return core::system::socket(domain, type | flags, protocol);
    }

    int Socket::open(Flags flags) {
        int ret = Descriptor::open(create(flags));

        if (ret >= 0) {
            setSockopt();
        }

        return ret;
    }

    void Socket::setSockopt() {
    }

    bool Socket::isValid() const {
        return getFd() >= 0;
    }

    int Socket::reuseAddress() const {
        int sockopt = 1;
        return setSockopt(SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    }

    int Socket::reusePort() const {
        int sockopt = 1;
        return setSockopt(SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt));
    }

    int Socket::getSockError() const {
        int cErrno = 0;
        socklen_t cErrnoLen = sizeof(cErrno);
        int err = getSockopt(SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

        return err < 0 ? err : cErrno;
    }

    int Socket::setSockopt(int level, int optname, const void* optval, socklen_t optlen) const {
        return core::system::setsockopt(Socket::getFd(), level, optname, optval, optlen);
    }

    int Socket::getSockopt(int level, int optname, void* optval, socklen_t* optlen) const {
        return core::system::getsockopt(Socket::getFd(), level, optname, optval, optlen);
    }

    void Socket::shutdown(SHUT how) {
        core::system::shutdown(getFd(), how);
    }

} // namespace core::socket
