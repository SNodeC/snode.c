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

#include "net/stream/ServerSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    template <typename Socket>
    int ServerSocket<Socket>::listen([[maybe_unused]] int backlog) {
        return core::system::listen(Socket::getFd(), backlog);
    }

    template <typename Socket>
    int ServerSocket<Socket>::accept(sockaddr* addr, socklen_t* addrlen) {
        return core::system::accept(Socket::getFd(), addr, addrlen);
    }

    template <typename Socket>
    int ServerSocket<Socket>::accept4(sockaddr* addr, socklen_t* addrlen, int flags) {
        return core::system::accept4(Socket::getFd(), addr, addrlen, flags);
    }

} // namespace net::stream
