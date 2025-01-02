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

#include "net/in/SocketAddress.h"

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"
#include "net/in/SocketAddrInfo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    SocketAddress::SocketAddress()
        : Super(AF_INET)
        , socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname)
        : SocketAddress() {
        setHost(ipOrHostname);
    }

    SocketAddress::SocketAddress(uint16_t port)
        : SocketAddress() {
        setPort(port);
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname, uint16_t port)
        : SocketAddress() {
        setHost(ipOrHostname);
        setPort(port);
    }

    SocketAddress::SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen, bool numeric)
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen)
        , socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
        char hostC[NI_MAXHOST];
        char servC[NI_MAXSERV];
        std::memset(hostC, 0, NI_MAXHOST);
        std::memset(servC, 0, NI_MAXSERV);

        const int aiErrCode = core::system::getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr),
                                                        sizeof(sockAddr),
                                                        hostC,
                                                        NI_MAXHOST,
                                                        servC,
                                                        NI_MAXSERV,
                                                        NI_NUMERICSERV | (numeric ? NI_NUMERICHOST : NI_NAMEREQD));
        if (aiErrCode == 0) {
            if (servC[0] != '\0') {
                this->port = static_cast<uint16_t>(std::stoul(servC));
            }

            this->host = hostC;
            this->canonName = hostC;
        } else {
            core::socket::State state = core::socket::STATE_OK;
            switch (aiErrCode) {
                case EAI_AGAIN:
                case EAI_MEMORY:
                    state = core::socket::STATE_ERROR;
                    break;
                default:
                    state = core::socket::STATE_FATAL;
                    break;
            }

            throw core::socket::SocketAddress::BadSocketAddress(
                state, aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode), aiErrCode == EAI_SYSTEM ? errno : aiErrCode);
        }
    }

    void SocketAddress::init(const Hints& hints) {
        addrinfo addrInfoHints{};

        addrInfoHints.ai_family = Super::getAddressFamily();
        addrInfoHints.ai_flags =
            hints.aiFlags | AI_ADDRCONFIG | AI_CANONNAME /*| AI_CANONIDN*/; // AI_CANONIDN produces a still reachable memory leak
        addrInfoHints.ai_socktype = hints.aiSockType;
        addrInfoHints.ai_protocol = hints.aiProtocol;

        const int aiErrCode = socketAddrInfo->resolve(host, std::to_string(port), addrInfoHints);
        if (aiErrCode == 0) {
            sockAddr = socketAddrInfo->getSockAddr();
            canonName = socketAddrInfo->getCanonName();
        } else {
            core::socket::State state = core::socket::STATE_OK;

            switch (aiErrCode) {
                case EAI_AGAIN:
                case EAI_MEMORY:
                    state = core::socket::STATE_ERROR;
                    break;
                default:
                    state = core::socket::STATE_FATAL;
                    break;
            }

            throw core::socket::SocketAddress::BadSocketAddress(state,
                                                                host + ":" + std::to_string(port) + " - " +
                                                                    (aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),
                                                                (aiErrCode == EAI_SYSTEM ? errno : aiErrCode));
        }
    }

    SocketAddress& SocketAddress::setHost(const std::string& ipOrHostname) {
        this->host = ipOrHostname;

        return *this;
    }

    std::string SocketAddress::getHost() const {
        return host;
    }

    SocketAddress& SocketAddress::setPort(uint16_t port) {
        this->port = port;

        return *this;
    }

    uint16_t SocketAddress::getPort() const {
        return port;
    }

    std::string SocketAddress::getCanonName() const {
        return canonName;
    }

    std::string SocketAddress::toString(bool expanded) const {
        return std::string(host)
            .append(":")
            .append(std::to_string(port))
            .append(expanded ? std::string(" (").append(canonName).append(")") : "");
    }

    bool SocketAddress::useNext() {
        const bool useNext = socketAddrInfo->useNext();

        sockAddr = socketAddrInfo->getSockAddr();

        return useNext;
    }

} // namespace net::in

template class net::SocketAddress<sockaddr_in>;
