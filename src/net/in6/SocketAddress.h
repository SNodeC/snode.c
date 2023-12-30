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

#ifndef NET_IN6_SOCKETADDRESS_H
#define NET_IN6_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

namespace net::in6 {
    class SocketAddrInfo;
} // namespace net::in6

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6 {

    class SocketAddress final : public net::SocketAddress<sockaddr_in6> {
    private:
        using Super = net::SocketAddress<sockaddr_in6>;

    public:
        struct Hints {
            int aiFlags = 0;
            int aiSockType = 0;
            int aiProtocol = 0;
        };

        using Super::Super;

        SocketAddress(const Hints& options = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0}); // cppcheck-suppress noExplicitConstructor
        explicit SocketAddress(const std::string& ipOrHostname, const Hints& options = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0});
        explicit SocketAddress(uint16_t port, const Hints& options = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0});
        SocketAddress(const std::string& ipOrHostname, // cppcheck-suppress noExplicitConstructor // wrong positive
                      uint16_t port,
                      const Hints& options = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0});
        SocketAddress(const SocketAddress::SockAddr& sockAddr, socklen_t sockAddrLen);

        SocketAddress& init();

        bool useNext() override;

        SocketAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        SocketAddress& setPort(uint16_t port);
        uint16_t getPort() const;

        std::string toString() const override;

    private:
        std::string host = "::";
        uint16_t port = 0;

        std::string canonName;

        int aiSockType = 0;
        int aiProtocol = 0;
        int aiFlags = 0;

        std::shared_ptr<SocketAddrInfo> socketAddrInfo;
    };

} // namespace net::in6

extern template class net::SocketAddress<sockaddr_in6>;

#endif // NET_IN6_SOCKETADDRESS_H
