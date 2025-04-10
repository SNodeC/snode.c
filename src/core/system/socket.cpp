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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int socket(int domain, int type, int protocol) {
        errno = 0;
        return ::socket(domain, type, protocol);
    }

    int bind(int sockfd, const sockaddr* addr, socklen_t addrlen) {
        errno = 0;
        return ::bind(sockfd, addr, addrlen);
    }

    int listen(int sockfd, int backlog) {
        errno = 0;
        return ::listen(sockfd, backlog);
    }

    int accept(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        errno = 0;
        return ::accept(sockfd, addr, addrlen);
    }

    int accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags) {
        errno = 0;
        return ::accept4(sockfd, addr, addrlen, flags);
    }
    int connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
        errno = 0;
        return ::connect(sockfd, addr, addrlen);
    }

    int getsockname(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        errno = 0;
        return ::getsockname(sockfd, addr, addrlen);
    }

    int getpeername(int sockfd, sockaddr* addr, socklen_t* addrlen) {
        errno = 0;
        return ::getpeername(sockfd, addr, addrlen);
    }

    ssize_t recv(int sockfd, void* buf, std::size_t len, int flags) {
        errno = 0;
        return ::recv(sockfd, buf, len, flags);
    }

    ssize_t send(int sockfd, const void* buf, std::size_t len, int flags) {
        errno = 0;
        return ::send(sockfd, buf, len, flags);
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
        errno = 0;
        return ::getsockopt(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
        errno = 0;
        return ::setsockopt(sockfd, level, optname, optval, optlen);
    }

    int shutdown(int sockfd, int how) {
        errno = 0;
        return ::shutdown(sockfd, how);
    }

} // namespace core::system
