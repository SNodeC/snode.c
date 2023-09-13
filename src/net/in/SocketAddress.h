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

#ifndef NET_IN_SOCKETADDRESS_H
#define NET_IN_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

namespace net::in {
    class SocketAddrInfo;
}

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    class SocketAddress final : public net::SocketAddress<sockaddr_in> {
    public:
        using net::SocketAddress<sockaddr_in>::SocketAddress;

        SocketAddress();
        explicit SocketAddress(const std::string& ipOrHostname);
        SocketAddress(const std::string& ipOrHostname, uint16_t port);
        explicit SocketAddress(uint16_t port);

        SocketAddress(const SocketAddress::SockAddr& sockAddr, socklen_t sockAddrLen);

        SocketAddress& setHost(const std::string& ipOrHostname);
        SocketAddress& setPort(uint16_t port);

        uint16_t getPort() const;
        std::string getHost() const;
        std::string getServ() const;

        std::string address() const override;
        std::string toString() const override;

        SocketAddress& setAiFlags(int aiFlags);
        SocketAddress& setAiSocktype(int aiSocktype);
        SocketAddress& setAiProtocol(int aiProtocol);

        const sockaddr& getSockAddr() override;

        bool hasNext() override;

    private:
        std::shared_ptr<SocketAddrInfo> socketAddrInfo;
        int aiFlags = 0;
        int aiSocktype = 0;
        int aiProtocol = 0;

        std::string host = "";
        uint16_t port = 0;
        std::string serv = "";
    };

} // namespace net::in

#endif // NET_IN_SOCKETADDRESS_H
