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

#ifndef NET_CONFIG_CONFIGADDRESS_HPP
#define NET_CONFIG_CONFIGADDRESS_HPP

#include "net/config/ConfigAddress.h" // IWYU pragma: export
#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename SocketAddress>
    ConfigAddress<SocketAddress>::ConfigAddress(ConfigInstance* instance,
                                                const std::string& addressOptionName,
                                                const std::string& addressOptionDescription)
        : net::config::ConfigSection(instance, addressOptionName, addressOptionDescription) {
    }

    template <typename SocketAddress>
    ConfigAddress<SocketAddress>::~ConfigAddress() {
        delete socketAddress;
    }

    template <typename SocketAddress>
    SocketAddress ConfigAddress<SocketAddress>::newSocketAddress(const SocketAddress::SockAddr& sockAddr,
                                                                 SocketAddress::SockLen sockAddrLen) {
        return SocketAddress(sockAddr, sockAddrLen);
    }

    template <typename SocketAddress>
    SocketAddress& ConfigAddress<SocketAddress>::getSocketAddress() {
        if (socketAddress == nullptr) {
            socketAddress = init();
        }

        return *socketAddress;
    }

    template <typename SocketAddress>
    void ConfigAddress<SocketAddress>::renew() {
        if (socketAddress != nullptr) {
            delete socketAddress;
            socketAddress = nullptr;
        }
    }

} // namespace net::config

#endif // NET_CONFIG_CONFIGADDRESS_HPP
