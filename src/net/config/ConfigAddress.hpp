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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename SocketAddress>
    ConfigAddress<SocketAddress>::ConfigAddress(const std::string& addressOptionName, const std::string& addressOptionDescription)
        : net::config::ConfigSection(addressOptionName, addressOptionDescription) {
        _initialized = !getInstanceName().empty();
    }

    template <typename SocketAddress>
    bool ConfigAddress<SocketAddress>::isInitialized() const {
        return _initialized;
    }

    template <typename SocketAddress>
    void ConfigAddress<SocketAddress>::initialized() {
        required(false);
        _initialized = true;
    }

    template <typename SocketAddress>
    void ConfigAddress<SocketAddress>::require(CLI::Option* opt) {
        required();
        opt->default_str("")->required()->clear();
    }

} // namespace net::config

#endif // NET_CONFIG_CONFIGADDRESS_HPP
