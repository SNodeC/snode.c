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

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        hostOpt = ConfigAddressType::addressSc->add_option("--host", host, "Bluetooth address");
        hostOpt->type_name("[bt address]");
        hostOpt->default_val("00:00:00:00:00:00");

        channelOpt = ConfigAddressType::addressSc->add_option("--channel", channel, "Channel number");
        channelOpt->type_name("[uint8_t]");
        channelOpt->default_val(0);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        ConfigAddressType::require(hostOpt, channelOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::channelRequired() {
        ConfigAddressType::require(channelOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::updateFromCommandLine() {
        if (hostOpt->count() > 0) {
            ConfigAddressType::address.setAddress(host);
        }
        if (channelOpt->count() > 0) {
            ConfigAddressType::address.setChannel(channel);
        }
    }

    template class ConfigAddress<net::config::ConfigAddressLocal>;
    template class ConfigAddress<net::config::ConfigAddressRemote>;

} // namespace net::rf::config

namespace net::config {

    template class ConfigAddressLocal<net::rf::SocketAddress>;
    template class ConfigAddressRemote<net::rf::SocketAddress>;

} // namespace net::config
