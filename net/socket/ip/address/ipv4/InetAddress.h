/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_IP_ADDRESS_IPV4_INETADDRESS_H
#define NET_SOCKET_IP_ADDRESS_IPV4_INETADDRESS_H

#include "net/socket/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <exception>    // IWYU pragma: keep
#include <netinet/in.h> // IWYU pragma: keep
#include <string>
// IWYU pragma: no_include <bits/exception.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::ip::address::ipv4 {

    class bad_hostname : public std::exception {
    public:
        explicit bad_hostname(const std::string& hostName) {
            bad_hostname::message = "Bad hostname \"" + hostName + "\"";
        }

        const char* what() const noexcept override {
            return bad_hostname::message.c_str();
        }

    protected:
        static std::string message;
    };

    class InetAddress : public SocketAddress<struct sockaddr_in> {
    public:
        using SocketAddress<struct sockaddr_in>::SocketAddress;

        explicit InetAddress(const std::string& ipOrHostname);
        InetAddress(const std::string& ipOrHostname, uint16_t port);
        explicit InetAddress(uint16_t port);

        uint16_t port() const;
        std::string host() const;
        std::string ip() const;
        std::string serv() const;

        std::string toString() const override;
    };

} // namespace net::socket::ip::address::ipv4

#endif // NET_SOCKET_IP_ADDRESS_IPV4_INETADDRESS_H
