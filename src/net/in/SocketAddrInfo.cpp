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

    int SocketAddrInfo::init(const std::string& node, const std::string& service, const addrinfo& hints) {
        int err = 0;

        this->node = node;
        this->service = service;

        if (addrInfo != nullptr) {
            freeaddrinfo(addrInfo);
            addrInfo = nullptr;
        }

        if ((err = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &addrInfo)) == 0) {
            currentAddrInfo = addrInfo;
            SocketAddrInfo::logAddressInfo("Initialized AddressInfo", currentAddrInfo);
        }

        return err;
    }

    bool SocketAddrInfo::useNext() {
        bool useNext = false;

        if (currentAddrInfo != nullptr && currentAddrInfo->ai_next != nullptr) {
            currentAddrInfo = currentAddrInfo->ai_next;
            useNext = true;
        } else {
            currentAddrInfo = addrInfo;
        }

        LOG(TRACE) << "Has Next: " << useNext;

        if (useNext) {
            SocketAddrInfo::logAddressInfo("Next potentially used AddressInfo", currentAddrInfo);
        }

        return useNext;
    }

    const sockaddr* SocketAddrInfo::getSockAddr() {
        return currentAddrInfo != nullptr ? currentAddrInfo->ai_addr : nullptr;
    }

    void SocketAddrInfo::logAddressInfo(const std::string& title, const addrinfo* addrInfo) {
        if (addrInfo != nullptr) {
            el::Logger* defaultLogger = el::Loggers::getLogger("default");
            static char hostBfr[NI_MAXHOST];
            static char servBfr[NI_MAXSERV];
            std::memset(hostBfr, 0, NI_MAXHOST);
            std::memset(servBfr, 0, NI_MAXSERV);

            getnameinfo(addrInfo->ai_addr,
                        addrInfo->ai_addrlen,
                        hostBfr,
                        sizeof(hostBfr),
                        servBfr,
                        sizeof(servBfr),
                        NI_NUMERICHOST | NI_NUMERICSERV);

            struct sockaddr_in* aiAddr = (struct sockaddr_in*) addrInfo->ai_addr;

            std::string format = title + ":\n"
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

            defaultLogger->trace(format.c_str(),
                                 addrInfo->ai_next,
                                 addrInfo->ai_flags,
                                 addrInfo->ai_family,
                                 PF_INET,
                                 PF_INET6,
                                 addrInfo->ai_socktype,
                                 SOCK_STREAM,
                                 SOCK_DGRAM,
                                 addrInfo->ai_protocol,
                                 IPPROTO_TCP,
                                 IPPROTO_UDP,
                                 addrInfo->ai_addrlen,
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
