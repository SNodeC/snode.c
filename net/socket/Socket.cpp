/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"

namespace net::socket {

    Socket::~Socket() {
        if (!dontClose) {
            ::shutdown(getFd(), SHUT_RDWR);
        }
    }

    void Socket::open(int fd) {
        this->fd = fd;
    }

    void Socket::open(const std::function<void(int errnum)>& onError, int flags) {
        int fd = ::socket(AF_INET, SOCK_STREAM | flags, 0);

        if (fd >= 0) {
            open(fd);
            onError(0);
        } else {
            onError(errno);
        }
    }

    InetAddress& Socket::getLocalAddress() {
        return localAddress;
    }

    void Socket::bind(const InetAddress& localAddress, const std::function<void(int errnum)>& onError) {
        socklen_t addrlen = sizeof(struct sockaddr_in);

        int ret = ::bind(getFd(), reinterpret_cast<const struct sockaddr*>(&localAddress.getSockAddr()), addrlen);

        if (ret < 0) {
            onError(errno);
        } else {
            onError(0);
        }
    }

    void Socket::setLocalAddress(const InetAddress& localAddress) {
        this->localAddress = localAddress;
    }

} // namespace net::socket
