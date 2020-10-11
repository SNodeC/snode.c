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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <netdb.h>
#include <sys/socket.h> // for AF_INET

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"

namespace net::socket::ipv4 {

    std::string bad_hostname::message = "";

    InetAddress::InetAddress() {
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(0);
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    InetAddress::InetAddress(const std::string& ipOrHostname, uint16_t port) {
        struct hostent* he = gethostbyname(ipOrHostname.c_str());

        if (he == nullptr) {
            throw bad_hostname(ipOrHostname);
        }

        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(port);
        memcpy(&sockAddr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    InetAddress::InetAddress(const std::string& ipOrHostname) {
        struct hostent* he = gethostbyname(ipOrHostname.c_str());

        if (he == nullptr) {
            throw bad_hostname(ipOrHostname);
        }

        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(0);
        memcpy(&sockAddr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    InetAddress::InetAddress(unsigned short port) {
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(port);
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    InetAddress::InetAddress(const struct sockaddr_in& addr) {
        memcpy(&this->sockAddr, &addr, sizeof(struct sockaddr_in));
    }

    InetAddress::InetAddress(const InetAddress& ina) {
        memcpy(&sockAddr, &ina.sockAddr, sizeof(struct sockaddr_in));
    }

    InetAddress& InetAddress::operator=(const InetAddress& ina) {
        if (this != &ina) {
            memcpy(&sockAddr, &ina.sockAddr, sizeof(struct sockaddr_in));
        }

        return *this;
    }

    unsigned short InetAddress::port() const {
        return (ntohs(sockAddr.sin_port));
    }

    std::string InetAddress::host() const {
        char host[256];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), host, 256, NULL, 0, 0);

        return std::string(host);
    }

    std::string InetAddress::ip() const {
        char ip[256];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), ip, 256, NULL, 0, NI_NUMERICHOST);

        return std::string(ip);
    }

    std::string InetAddress::serv() const {
        char serv[256];
        getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr), NULL, 0, serv, 256, 0);

        return std::string(serv);
    }

    const struct sockaddr_in& InetAddress::getSockAddrIn() const {
        return sockAddr;
    }

} // namespace net::socket::ipv4
