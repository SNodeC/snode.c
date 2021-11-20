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

#include "core/system/netdb.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {
        errno = 0;
        return ::getaddrinfo(node, service, hints, res);
    }

    void freeaddrinfo(struct addrinfo* res) {
        errno = 0;
        return ::freeaddrinfo(res);
    }

    int
    getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags) {
        errno = 0;
        return ::getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
    }

} // namespace net::system
