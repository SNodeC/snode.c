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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int socket(int domain, int type, int protocol) {
        return ::socket(domain, type, protocol);
    }

    int bind(int sockfd, const sockaddr* addr, socklen_t addrlen) {
        return ::bind(sockfd, addr, addrlen);
    }

    int listen(int sockfd, int backlog) {
        return ::listen(sockfd, backlog);
    }

    int accept(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        return ::accept(sockfd, addr, addrlen);
    }

    int accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags) {
        return ::accept4(sockfd, addr, addrlen, flags);
    }
    int connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
        return ::connect(sockfd, addr, addrlen);
    }

    int getsockname(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        return ::getsockname(sockfd, addr, addrlen);
    }

    int getpeername(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        return ::getpeername(sockfd, addr, addrlen);
    }

    ssize_t recv(int sockfd, void* buf, std::size_t len, int flags) {
        return ::recv(sockfd, buf, len, flags);
    }

    ssize_t send(int sockfd, const void* buf, std::size_t len, int flags) {
        return ::send(sockfd, buf, len, flags);
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
        return ::getsockopt(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
        return ::setsockopt(sockfd, level, optname, optval, optlen);
    }

    int shutdown(int sockfd, int how) {
        return ::shutdown(sockfd, how);
    }

} // namespace core::system
