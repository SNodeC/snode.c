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

#include "core/system/netdb.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// Workaround for https://android.googlesource.com/platform/bionic/+/refs/heads/main/libc/include/netdb.h#205
#ifdef __ANDROID__
#include <cstddef>
#define HOSTLEN_CAST(A) static_cast<std::size_t>((A))
#define SERVLEN_CAST(A) static_cast<std::size_t>((A))
#else
#define HOSTLEN_CAST(A) ((A))
#define SERVLEN_CAST(A) ((A))
#endif

namespace core::system {

    int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {
        errno = 0;
        return ::getaddrinfo(node, service, hints, res);
    }

    void freeaddrinfo(struct addrinfo* res) {
        errno = 0;
        ::freeaddrinfo(res);
    }

    int getnameinfo(const sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags) {
        errno = 0;
        return ::getnameinfo(addr, addrlen, host, HOSTLEN_CAST(hostlen), serv, SERVLEN_CAST(servlen), flags);
    }

} // namespace core::system
