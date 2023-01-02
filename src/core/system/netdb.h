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

#ifndef NET_SYSTEM_NETDB_H
#define NET_SYSTEM_NETDB_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    // #include <sys/types.h>, #include <sys/socket.h>, #include <netdb.h>
    int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);

    // #include <sys/types.h>, #include <sys/socket.h>, #include <netdb.h>
    void freeaddrinfo(struct addrinfo* res);

    // #include <sys/socket>, #include <netdb.h>
    int getnameinfo(const sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);

} // namespace core::system

#endif // NET_SYSTEM_NETDB_H
