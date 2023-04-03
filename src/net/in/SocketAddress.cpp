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

#include "net/in/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/netdb.h"
#include "utils/PreserveErrno.h"

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    SocketAddress::SocketAddress() {
        sockAddr.sin_family = AF_INET;
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname)
        : SocketAddress() {
        setHost(ipOrHostname);
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname, uint16_t port)
        : SocketAddress() {
        setHost(ipOrHostname);
        setPort(port);
    }

    SocketAddress::SocketAddress(uint16_t port)
        : SocketAddress() {
        setPort(port);
    }

    void SocketAddress::setHost(const std::string& ipOrHostname) {
        struct addrinfo hints {};
        struct addrinfo* res = nullptr;
        struct addrinfo* resalloc = nullptr;

        memset(&hints, 0, sizeof(hints));

        /* We only care about IPV results */
        hints.ai_family = AF_INET;
        hints.ai_socktype = 0;
        hints.ai_flags = AI_ADDRCONFIG;

        int err = core::system::getaddrinfo(ipOrHostname.c_str(), nullptr, &hints, &res);

        if (err != 0) {
            throw net::BadSocketAddress("IPv4 error not resolvable: " + ipOrHostname);
        }

        resalloc = res;

        while (res) {
            /* Check to make sure we have a valid AF_INET address */
            if (res->ai_family == AF_INET) {
                sockAddr.sin_addr.s_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr.s_addr;
                break;
            }

            res = res->ai_next;
        }

        core::system::freeaddrinfo(resalloc);
    }

    void SocketAddress::setPort(uint16_t port) {
        sockAddr.sin_port = htons(port);
    }

    uint16_t SocketAddress::port() const {
        return (ntohs(sockAddr.sin_port));
    }

    std::string SocketAddress::host() const {
        utils::PreserveErrno preserveErrno;

        char host[NI_MAXHOST];
        int ret = core::system::getnameinfo(
            reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD);

        return ret == EAI_NONAME ? address() : ret >= 0 ? host : gai_strerror(ret);
    }

    std::string SocketAddress::address() const {
        utils::PreserveErrno preserveErrno;

        char ip[NI_MAXHOST];
        int ret = core::system::getnameinfo(
            reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), ip, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

        return ret >= 0 ? ip : gai_strerror(ret);
    }

    std::string SocketAddress::serv() const {
        utils::PreserveErrno preserveErrno;

        char serv[NI_MAXSERV];
        int ret =
            core::system::getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), nullptr, 0, serv, NI_MAXSERV, 0);

        return ret >= 0 ? serv : gai_strerror(ret);
    }

    std::string SocketAddress::toString() const {
        return host() + ":" + std::to_string(port());
    }

} // namespace net::in

template class net::SocketAddress<sockaddr_in>;
