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

#ifndef NET_IN_SOCKETADDRINFO_H
#define NET_IN_SOCKETADDRINFO_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/netdb.h" // IWYU pragma: export

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    class SocketAddrInfo {
    public:
        SocketAddrInfo() = default;
        ~SocketAddrInfo();

        int init(const std::string& node, const std::string& service, const addrinfo& hints);

        bool hasNext();
        const sockaddr* getSockAddr();

    private:
        static void logAddressInfo(const std::string& title, const addrinfo* addrInfo);

        struct addrinfo* addrInfo = nullptr;
        struct addrinfo* currentAddrInfo = nullptr;

        std::string node;
        std::string service;
    };

} // namespace net::in

#endif // NET_IN_SOCKETADDRINFO_H
