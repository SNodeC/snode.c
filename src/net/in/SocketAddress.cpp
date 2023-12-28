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

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"
#include "net/in/SocketAddrInfo.h"

// IWYU pragma: no_include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    SocketAddress::SocketAddress(int aiFlags, int aiSockType, int aiProtocol)
        : Super(AF_INET)
        , aiSockType(aiSockType)
        , aiProtocol(aiProtocol)
        , aiFlags(aiFlags)
        , socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname, int aiFlags, int aiSockType, int aiProtocol)
        : SocketAddress(aiFlags, aiSockType, aiProtocol) {
        setHost(ipOrHostname);
    }

    SocketAddress::SocketAddress(uint16_t port, int aiFlags, int aiSockType, int aiProtocol)
        : SocketAddress(aiFlags, aiSockType, aiProtocol) {
        setPort(port);
    }

    SocketAddress::SocketAddress(const std::string& ipOrHostname, uint16_t port, int aiFlags, int aiSockType, int aiProtocol)
        : SocketAddress(aiFlags, aiSockType, aiProtocol) {
        setHost(ipOrHostname);
        setPort(port);
    }

    SocketAddress::SocketAddress(const SockAddr& sockAddr, socklen_t sockAddrLen)
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen)
        , socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        std::memset(host, 0, NI_MAXHOST);
        std::memset(serv, 0, NI_MAXSERV);

        int ret = core::system::getnameinfo(
            reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), nullptr, 0, serv, NI_MAXSERV, NI_NUMERICSERV);

        if (ret == 0) {
            ret = core::system::getnameinfo(
                reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD);
        }
        if (ret != 0) {
            ret = core::system::getnameinfo(
                reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
        }

        if (ret == 0) {
            if (serv[0] != '\0') {
                this->port = static_cast<uint16_t>(std::stoul(serv));
            }
            this->host = host;
            this->canonName = host;
        } else {
            this->host = gai_strerror(ret);
        }
    }

    SocketAddress& SocketAddress::init() {
        addrinfo hints{};

        hints.ai_family = Super::getAddressFamily();
        hints.ai_socktype = aiSockType;
        hints.ai_protocol = aiProtocol;
        hints.ai_flags = aiFlags;

        int aiErrCode = 0;

        if ((aiErrCode = socketAddrInfo->resolve(host, std::to_string(port), hints)) == 0) {
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

        return *this;
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

    std::string SocketAddress::toString() const {
        return std::string(host).append(":").append(std::to_string(port)).append(" (").append(canonName).append(")");
    }

    bool SocketAddress::useNext() {
        const bool useNext = socketAddrInfo->useNext();

        sockAddr = socketAddrInfo->getSockAddr();

        return useNext;
    }

} // namespace net::in

template class net::SocketAddress<sockaddr_in>;
