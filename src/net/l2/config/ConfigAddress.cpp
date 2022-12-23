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

#include "net/l2/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        if (!net::config::ConfigBase::getName().empty()) {
            hostOpt = ConfigAddressType::addressSc->add_option("--host", host, "Bluetooth address");
            hostOpt->type_name("[bt address]");
            hostOpt->default_val("00:00:00:00:00:00");

            psmOpt = ConfigAddressType::addressSc->add_option("--psm", psm, "Protocol service multiplexer");
            psmOpt->type_name("[uint16_t]");
            psmOpt->default_val(0);
        }
        ConfigAddressType::address.setAddress("00:00:00:00:00:00");
        ConfigAddressType::address.setPsm(0);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        ConfigAddressType::require(hostOpt, psmOpt);
        hostOpt->default_val("");
        psmOpt->default_val("");
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::psmRequired() {
        ConfigAddressType::require(psmOpt);
        psmOpt->default_val("");
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::updateFromCommandLine() {
        if (hostOpt->count() > 0) {
            ConfigAddressType::address.setAddress(host);
        }
        if (psmOpt->count() > 0) {
            ConfigAddressType::address.setPsm(psm);
        }
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::addressDefaultsFromCurrent() {
        hostOpt->default_val(ConfigAddressType::address.address());
        psmOpt->default_val(ConfigAddressType::address.psm());
    }

} // namespace net::l2::config

template class net::l2::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::l2::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::l2::SocketAddress>;
template class net::config::ConfigAddressRemote<net::l2::SocketAddress>;
