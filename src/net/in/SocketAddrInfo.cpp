/*
 * SNode.C - a slim toolkit for network communication
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

#include "net/in/SocketAddrInfo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstring>
#include <netinet/in.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    SocketAddrInfo::~SocketAddrInfo() {
        if (addrInfo != nullptr) {
            freeaddrinfo(addrInfo);
        }
    }

    int SocketAddrInfo::resolve(const std::string& node, const std::string& service, const addrinfo& hints) {
        if (addrInfo != nullptr) {
            freeaddrinfo(addrInfo);
            addrInfo = nullptr;
        }

        int aiErrCode = 0;

        if ((aiErrCode = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &addrInfo)) == 0) {
            currentAddrInfo = addrInfo;
        }

        return aiErrCode;
    }

    bool SocketAddrInfo::useNext() {
        // Lookup for a AddrInfo holding a ai_next AddrInfo which differ from the current one.
        // Especially localhost on IPv4 can lead to more than one entries representing the same SockAddr.
        // Thus, do not use this further SockAddr.
        while (currentAddrInfo != nullptr && currentAddrInfo->ai_next != nullptr &&
               std::memcmp(currentAddrInfo->ai_addr, currentAddrInfo->ai_next->ai_addr, sizeof(sockaddr_in)) == 0) {
            currentAddrInfo = currentAddrInfo->ai_next;
        }

        if (currentAddrInfo != nullptr) {
            currentAddrInfo = currentAddrInfo->ai_next;
        }

        return currentAddrInfo != nullptr;
    }

    sockaddr_in SocketAddrInfo::getSockAddr() {
        return currentAddrInfo != nullptr ? *reinterpret_cast<sockaddr_in*>(currentAddrInfo->ai_addr) // cppcheck-suppress internalAstError
                                          : sockaddr_in{.sin_family = AF_INET, .sin_port{}, .sin_addr{}, .sin_zero{}};
    }

    std::string SocketAddrInfo::getCanonName() {
        return currentAddrInfo->ai_canonname != nullptr ? currentAddrInfo->ai_canonname : std::string{};
    }

    void SocketAddrInfo::logAddressInfo() {
        if (currentAddrInfo != nullptr) {
            static char hostBfr[NI_MAXHOST];
            static char servBfr[NI_MAXSERV];
            std::memset(hostBfr, 0, NI_MAXHOST);
            std::memset(servBfr, 0, NI_MAXSERV);

            getnameinfo(currentAddrInfo->ai_addr,
                        currentAddrInfo->ai_addrlen,
                        hostBfr,
                        sizeof(hostBfr),
                        servBfr,
                        sizeof(servBfr),
                        NI_NUMERICHOST | NI_NUMERICSERV);

            struct sockaddr_in* aiAddr = reinterpret_cast<sockaddr_in*>(currentAddrInfo->ai_addr);

            const std::string format = "AddressInfo:\n"
                                       "   ai_next      = %v\n"
                                       "   ai_flags     = %v\n"
                                       "   ai_family    = %v (PF_INET = %v, PF_INET6 = %v)\n"
                                       "   ai_socktype  = %v (SOCK_STREAM = %v, SOCK_DGRAM = %v)\n"
                                       "   ai_protocol  = %v (IPPROTO_TCP = %v, IPPROTO_UDP = %v)\n"
                                       "   ai_addrlen   = %v (sockaddr_in = %v, "
                                       "sockaddr_in6 = %v)\n"
                                       "   ai_addr      = sin_family:   %v (AF_INET = %v, "
                                       "AF_INET6 = %v)\n"
                                       "                  sin_addr:     %v\n"
                                       "                  sin_port:     %v";

            el::Loggers::getLogger("default")->trace(format.c_str(),
                                                     currentAddrInfo->ai_next,
                                                     currentAddrInfo->ai_flags,
                                                     currentAddrInfo->ai_family,
                                                     PF_INET,
                                                     PF_INET6,
                                                     currentAddrInfo->ai_socktype,
                                                     SOCK_STREAM,
                                                     SOCK_DGRAM,
                                                     currentAddrInfo->ai_protocol,
                                                     IPPROTO_TCP,
                                                     IPPROTO_UDP,
                                                     currentAddrInfo->ai_addrlen,
                                                     sizeof(struct sockaddr_in),
                                                     sizeof(struct sockaddr_in6),
                                                     aiAddr->sin_family,
                                                     AF_INET,
                                                     AF_INET6,
                                                     hostBfr,
                                                     servBfr);
        }
    }

} // namespace net::in
