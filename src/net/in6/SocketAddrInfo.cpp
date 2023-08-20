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

#include "net/in6/SocketAddrInfo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstring>
#include <netinet/in.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6 {

    SocketAddrInfo::~SocketAddrInfo() {
        if (addrInfo != nullptr) {
            freeaddrinfo(addrInfo);
        }
    }

    int SocketAddrInfo::init(const std::string& node, const std::string& service, const addrinfo& hints) {
        int err = 0;

        if (!node.empty() && !service.empty() && (this->node != node || this->service != service)) {
            this->node = node;
            this->service = service;

            if (addrInfo != nullptr) {
                freeaddrinfo(addrInfo);
                addrInfo = nullptr;
            }

            if ((err = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &addrInfo)) == 0) {
                currentAddrInfo = addrInfo;
            }
        }

        return err;
    }

    bool SocketAddrInfo::hasNext() {
        bool hasNext = false;

        if (currentAddrInfo != nullptr && currentAddrInfo->ai_next != nullptr) {
            currentAddrInfo = currentAddrInfo->ai_next;
            hasNext = true;
        } else {
            currentAddrInfo = addrInfo;
        }

        return hasNext;
    }

    addrinfo* SocketAddrInfo::getAddrInfo() {
        el::Logger* defaultLogger = el::Loggers::getLogger("default");
        static char hostBfr[NI_MAXHOST];
        static char servBfr[NI_MAXSERV];
        std::memset(hostBfr, 0, NI_MAXHOST);
        std::memset(servBfr, 0, NI_MAXSERV);

        if (currentAddrInfo != nullptr) {
            getnameinfo(currentAddrInfo->ai_addr,
                        currentAddrInfo->ai_addrlen,
                        hostBfr,
                        sizeof(hostBfr),
                        servBfr,
                        sizeof(servBfr),
                        NI_NUMERICHOST | NI_NUMERICSERV);

            struct sockaddr_in6* aiAddr = (struct sockaddr_in6*) currentAddrInfo->ai_addr;

            defaultLogger->trace("AddressInfo:\n"
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
                                 "                  sin_port:     %v\n"
                                 "                  sin6_flowinfo: %v\n"
                                 "                  sin6_scope_id: %v",
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
                                 aiAddr->sin6_family,
                                 AF_INET,
                                 AF_INET6,
                                 hostBfr,
                                 servBfr,
                                 aiAddr->sin6_flowinfo,
                                 aiAddr->sin6_scope_id);
        }

        return currentAddrInfo;
    }

} // namespace net::in6

/*
#include <cstdint>
#include <cstring>
#include <format>

         LOG(TRACE) << std::format("AddressInfo:\n"
                                   "   ai_next      = {:#010x}\n"
                                   "   ai_flags     = {:#2x}\n"
                                   "   ai_family    = {} (PF_INET = {}, PF_INET6 = {})\n"
                                   "   ai_socktype  = {} (SOCK_STREAM = {}, SOCK_DGRAM = {})\n"
                                   "   ai_protocol  = {} (IPPROTO_TCP = {}, IPPROTO_UDP = {})\n"
                                   "   ai_addrlen   = {} (sockaddr_in = {}, "
                                   "sockaddr_in6 = {})\n"
                                   "   ai_addr      = sin_family:   {} (AF_INET = {}, "
                                   "AF_INET6 = {})\n"
                                   "                  sin_addr:     {}\n"
                                   "                  sin_port:     {}\n"
                                   "                  sin6_flowinfo: {}\n"
                                   "                  sin6_scope_id: {}",
                                   reinterpret_cast<intptr_t>(currentAddrInfo->ai_next),
                                   currentAddrInfo->ai_flags,
                                   currentAddrInfo->ai_family,
                                   PF_INET,
                                   PF_INET6,
                                   currentAddrInfo->ai_socktype,
                                   static_cast<int>(SOCK_STREAM),
                                   static_cast<int>(SOCK_DGRAM),
                                   currentAddrInfo->ai_protocol,
                                   static_cast<int>(IPPROTO_TCP),
                                   static_cast<int>(IPPROTO_UDP),
                                   currentAddrInfo->ai_addrlen,
                                   sizeof(struct sockaddr_in),
                                   sizeof(struct sockaddr_in6),
                                   aiAddr->sin6_family,
                                   AF_INET,
                                   AF_INET6,
                                   hostBfr,
                                   servBfr,
                                   aiAddr->sin6_flowinfo,
                                   aiAddr->sin6_scope_id);
*/
