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

#include "net/in6/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/ResetValidator.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        if (!net::config::ConfigInstance::getInstanceName().empty()) {
            hostOpt = //
                ConfigAddressType::addressSc
                    ->add_option("--host", host, "Host name or IPv6 address") //
                    ->type_name("hostname|IPv6")                              //
                    ->default_val("::")                                       //
                    ->check(utils::ResetValidator(hostOpt));

            portOpt = //
                ConfigAddressType::addressSc
                    ->add_option("--port", port, "Port number") //
                    ->type_name("uint_16")                      //
                    ->default_val(0)                            //
                    ->check(utils::ResetValidator(portOpt));
        }
        ConfigAddressType::address.setHost("::");
        ConfigAddressType::address.setPort(0);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        portRequired();
        ConfigAddressType::require(hostOpt);
        host = "";
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::portRequired() {
        ConfigAddressType::require(portOpt);
        port = 0;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::updateFromCommandLine() {
        if (hostOpt->count() > 0) {
            ConfigAddressType::address.setHost(host);
        }
        if (portOpt->count() > 0) {
            ConfigAddressType::address.setPort(port);
        }
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::addressDefaultsFromCurrent() {
        hostOpt //
            ->default_val(ConfigAddressType::address.host())
            ->required(false)
            ->clear();
        portOpt //
            ->default_val(ConfigAddressType::address.port())
            ->required(false)
            ->clear();
    }

} // namespace net::in6::config

template class net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::in6::SocketAddress>;
template class net::config::ConfigAddressRemote<net::in6::SocketAddress>;
