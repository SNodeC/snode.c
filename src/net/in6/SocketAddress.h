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

#ifndef NET_IN6_SOCKETADDRESS_H
#define NET_IN6_SOCKETADDRESS_H

#include "net/SocketAddress.h"

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <exception>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6 {

    class bad_hostname : public std::exception {
    public:
        explicit bad_hostname(const std::string& hostName);

        const char* what() const noexcept override;

    protected:
        static std::string message;
    };

    class SocketAddress : public net::SocketAddress<struct sockaddr_in6> {
    public:
        using net::SocketAddress<struct sockaddr_in6>::SocketAddress;

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

} // namespace net::in6

#endif // NET_IN6_SOCKETADDRESS_H
