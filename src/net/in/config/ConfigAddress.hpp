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

#include "ConfigAddress.h"
#include "net/config/ConfigLocal.hpp"
#include "net/config/ConfigRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(CLI::App* baseSc)
        : ConfigAddressType(baseSc) {
        hostOpt = ConfigAddressType::addressSc->add_option("-a,--host", host, "Host name or IP address");
        hostOpt->type_name("[hostname|ip]");
        hostOpt->default_val("0.0.0.0");
        hostOpt->take_first();
        hostOpt->configurable();

        portOpt = ConfigAddressType::addressSc->add_option("-p,--port", port, "Port number");
        portOpt->type_name("[uint16_t]");
        portOpt->default_val(0);
        portOpt->take_first();
        portOpt->configurable();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        ConfigAddressType::require(hostOpt, portOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::portRequired() {
        ConfigAddressType::require(portOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getAddress() const {
        return SocketAddress(host, port);
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

} // namespace net::in::config

namespace net::config {
    template class ConfigLocal<net::in::SocketAddress>;
    template class ConfigRemote<net::in::SocketAddress>;
} // namespace net::config
