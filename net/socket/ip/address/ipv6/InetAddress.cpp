/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "net/socket/ip/address/ipv6/InetAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <netdb.h>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::ip::address::ipv6 {

    std::string bad_hostname::message;

    InetAddress::InetAddress(const std::string& ipOrHostname)
        : InetAddress(ipOrHostname, 0) {
    }

    InetAddress::InetAddress(const std::string& ipOrHostname, uint16_t port) {
        struct addrinfo hints {};
        struct addrinfo* res;
        struct addrinfo* resalloc;

        memset(&hints, 0, sizeof(hints));
        memset(&sockAddr, 0, sizeof(sockAddr));

        /* We only care about IPV6 results */
        hints.ai_family = AF_INET6;
        hints.ai_socktype = 0;
        hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;

        int err = getaddrinfo(ipOrHostname.c_str(), nullptr, &hints, &res);

        if (err != 0) {
            throw bad_hostname(ipOrHostname);
        }

        resalloc = res;

        while (res) {
            /* Check to make sure we have a valid AF_INET6 address */
            if (res->ai_family == AF_INET6) {
                /* Use memcpy since we're going to free the res variable later */
                memcpy(&sockAddr, res->ai_addr, res->ai_addrlen);

                /* Here we convert the port to network byte order */
                sockAddr.sin6_port = htons(port);
                sockAddr.sin6_family = AF_INET6;
                break;
            }

            res = res->ai_next;
        }

        freeaddrinfo(resalloc);
    }

    InetAddress::InetAddress(uint16_t port) {
        sockAddr.sin6_family = AF_INET6;
        sockAddr.sin6_addr = in6addr_any;
        sockAddr.sin6_port = htons(port);
    }

    uint16_t InetAddress::port() const {
        return (ntohs(sockAddr.sin6_port));
    }

    std::string InetAddress::host() const {
        char host[NI_MAXHOST];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, 256, nullptr, 0, 0);

        return host;
    }

    std::string InetAddress::ip() const {
        char ip[NI_MAXHOST];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), ip, 256, nullptr, 0, NI_NUMERICHOST);

        return ip;
    }

    std::string InetAddress::serv() const {
        char serv[NI_MAXSERV];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), nullptr, 0, serv, 256, 0);

        return serv;
    }

    std::string InetAddress::toString() const {
        return host() + "(" + ip() + "):" + std::to_string(port());
    }

} // namespace net::socket::ip::address::ipv6
