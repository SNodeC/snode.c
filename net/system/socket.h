/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <stddef.h> // for size_t
#include <sys/socket.h>
#include <sys/types.h> // for ssize_t

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <sys/socket.h>
    int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    int getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    int shutdown(int sockfd, int how);

    // #include <sys/types.h>, #include <sys/socket.h>
    int socket(int domain, int type, int protocol);
    int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    int listen(int sockfd, int backlog);
    int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags);
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    ssize_t recv(int sockfd, void* buf, size_t len, int flags);
    ssize_t send(int sockfd, const void* buf, size_t len, int flags);
    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

} // namespace net::system

#endif // SOCKET_H
