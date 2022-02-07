/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    class SocketAddress : public core::socket::SocketAddress<struct sockaddr_in> {
    public:
        using core::socket::SocketAddress<struct sockaddr_in>::SocketAddress;

        SocketAddress();
        explicit SocketAddress(const std::string& ipOrHostname);
        SocketAddress(const std::string& ipOrHostname, uint16_t port);
        explicit SocketAddress(uint16_t port);

        void setHost(const std::string& ipOrHostname);
        void setPort(uint16_t port);

        uint16_t port() const;
        std::string host() const;
        std::string serv() const;

        std::string address() const override;
        std::string toString() const override;
    };

} // namespace net::in

#endif // NET_IN_SOCKETADDRESS_H
