/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef NET_SYSTEM_SOCKET_H
#define NET_SYSTEM_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

// IWYU pragma: begin_exports

#include <sys/socket.h>
#include <sys/types.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    // #include <sys/socket.h>
    int getsockname(int sockfd, sockaddr* addr, socklen_t* addrlen);
    int getpeername(int sockfd, sockaddr* addr, socklen_t* addrlen);
    int shutdown(int sockfd, int how);

    // #include <sys/types.h>, #include <sys/socket.h>
    int socket(int domain, int type, int protocol);
    int bind(int sockfd, const sockaddr* addr, socklen_t addrlen);
    int listen(int sockfd, int backlog);
    int accept(int sockfd, sockaddr* addr, socklen_t* addrlen);
    int accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags);
    int connect(int sockfd, const sockaddr* addr, socklen_t addrlen);
    ssize_t recv(int sockfd, void* buf, std::size_t len, int flags);
    ssize_t send(int sockfd, const void* buf, std::size_t len, int flags);
    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

} // namespace core::system

#endif // NET_SYSTEM_SOCKET_H
