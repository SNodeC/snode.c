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

#include "net/in6/SocketAddress.h"

#include "net/SocketAddress.hpp"
#include "net/in6/SocketAddrInfo.h"

// IWYU pragma: no_include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <cerrno>
#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6 {

    SocketAddress::SocketAddress()
        : socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
        sockAddr.sin6_family = AF_INET6;
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

    SocketAddress::SocketAddress(const SockAddr& sockAddr, socklen_t sockAddrLen)
        : net::SocketAddress<SocketAddress::SockAddr>(sockAddr, sockAddrLen)
        , socketAddrInfo(std::make_shared<SocketAddrInfo>()) {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        std::memset(host, 0, NI_MAXHOST);
        std::memset(serv, 0, NI_MAXSERV);

        int ret = core::system::getnameinfo(
            reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), nullptr, 0, serv, NI_MAXSERV, NI_NUMERICSERV);

        if (ret == 0) {
            this->port = static_cast<uint16_t>(std::stoul(serv));

            ret = core::system::getnameinfo(
                reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD);
        }
        if (ret != 0) {
            this->host = host;

            ret = core::system::getnameinfo(
                reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
        }
        if (ret == 0) {
            this->host = host;
        } else {
            this->host = gai_strerror(ret);
        }
    }

    SocketAddress& SocketAddress::init() {
        addrinfo hints{};

        hints.ai_family = AF_INET6;
        hints.ai_socktype = aiSocktype;
        hints.ai_protocol = aiProtocol;
        hints.ai_flags = aiFlags | AI_ADDRCONFIG | AI_ALL;

        int aiErrCode = 0;

        if ((aiErrCode = socketAddrInfo->init(host, std::to_string(port), hints)) == 0) {
            const sockaddr* baseSockAddr = socketAddrInfo->getSockAddr();
            if (baseSockAddr != nullptr) {
                sockAddr = *reinterpret_cast<const SockAddr*>(socketAddrInfo->getSockAddr());
            } else {
                throw core::socket::SocketAddress::BadSocketAddress(
                    "IPv4 getaddrinfo for '" + host + "': " + (aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),
                    aiErrCode == EAI_SYSTEM ? errno : EINVAL);
            }
        } else {
            throw core::socket::SocketAddress::BadSocketAddress(
                "IPv4 init for '" + host + "': " + (aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),
                aiErrCode == EAI_SYSTEM ? errno : EINVAL);
        }

        return *this;
    }

    SocketAddress& SocketAddress::setHost(const std::string& ipOrHostname) {
        this->host = ipOrHostname;
        return *this;
    }

    SocketAddress& SocketAddress::setPort(uint16_t port) {
        this->port = port;
        return *this;
    }

    uint16_t SocketAddress::getPort() const {
        return port;
    }

    std::string SocketAddress::getHost() const {
        return host;
    }

    std::string SocketAddress::address() const {
        utils::PreserveErrno preserveErrno;

        char ip[NI_MAXHOST];
        int ret = core::system::getnameinfo(
            reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), ip, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

        return ret >= 0 ? ip : gai_strerror(ret);
    }

    std::string SocketAddress::getServ() const {
        utils::PreserveErrno preserveErrno;

        char serv[NI_MAXSERV];
        int ret =
            core::system::getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), nullptr, 0, serv, NI_MAXSERV, 0);

        return ret >= 0 ? serv : gai_strerror(ret);
    }

    std::string SocketAddress::toString() const {
        return getHost() + ":" + std::to_string(getPort());
    }

    SocketAddress& SocketAddress::setAiFlags(int aiFlags) {
        this->aiFlags = aiFlags;

        return *this;
    }

    SocketAddress& SocketAddress::setAiSocktype(int aiSocktype) {
        this->aiSocktype = aiSocktype;

        return *this;
    }

    SocketAddress& SocketAddress::setAiProtocol(int aiProtocol) {
        this->aiProtocol = aiProtocol;

        return *this;
    }

    const sockaddr& SocketAddress::getSockAddr() {
        sockAddr = *reinterpret_cast<const SockAddr*>(socketAddrInfo->getSockAddr());

        return reinterpret_cast<const sockaddr&>(sockAddr);
    }

    bool SocketAddress::hasNext() {
        return socketAddrInfo->hasNext();
    }

} // namespace net::in6

template class net::SocketAddress<sockaddr_in6>;
