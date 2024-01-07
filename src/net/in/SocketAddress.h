/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
} // namespace net::in

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    class SocketAddress final : public net::SocketAddress<sockaddr_in> {
    private:
        using Super = net::SocketAddress<sockaddr_in>;

    public:
        struct Hints {
            int aiFlags = 0;
            int aiSockType = 0;
            int aiProtocol = 0;
        };

        SocketAddress();
        explicit SocketAddress(const std::string& ipOrHostname);
        explicit SocketAddress(uint16_t port);
        SocketAddress(const std::string& ipOrHostname, uint16_t port);
        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen, bool numeric = true);

        SocketAddress& init(const Hints& hints = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0});

        bool useNext() override;

        SocketAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        SocketAddress& setPort(uint16_t port);
        uint16_t getPort() const;

        std::string getCanonName() const;

        std::string toString(bool simple = false) const override;

    private:
        std::string host = "0.0.0.0";
        uint16_t port = 0;

        std::string canonName;

        std::shared_ptr<SocketAddrInfo> socketAddrInfo;
    };

} // namespace net::in

extern template class net::SocketAddress<sockaddr_in>;

#endif // NET_IN_SOCKETADDRESS_H
