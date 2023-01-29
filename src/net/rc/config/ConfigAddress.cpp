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

#include "net/rc/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance)
        : Super(instance) {
        Super::add_option(
            hostOpt, "--host", "Bluetooth address", "xx:xx:xx:xx:xx:xx", "00:00:00:00:00:00", CLI::TypeValidator<std::string>());
        Super::add_option(channelOpt, "--channel", "Channel number", "channel", 0, CLI::Range(0, 255));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::hostRequired() {
        Super::required(hostOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::channelRequired() {
        Super::required(channelOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getAddress() const {
        utils::PreserveErrno preserveErrno;

        return SocketAddress(hostOpt->as<std::string>(), channelOpt->as<uint8_t>());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setAddress(const SocketAddress& socketAddress) {
        utils::PreserveErrno preserveErrno;

        hostOpt->default_val(socketAddress.address())->required(false)->clear();
        channelOpt->default_val(socketAddress.channel())->required(false)->clear();
    }

} // namespace net::rc::config

template class net::rc::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::rc::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::rc::SocketAddress>;
template class net::config::ConfigAddressRemote<net::rc::SocketAddress>;
