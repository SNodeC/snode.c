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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/ResetValidator.h"

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        hostOpt = //
            Super::addressSc
                ->add_option("--host", "Bluetooth address") //
                ->type_name("xx:xx:xx:xx:xx:xx")            //
                ->default_val("00:00:00:00:00:00")          //
                ->check(utils::ResetValidator(hostOpt));

        channelOpt = //
            Super::addressSc
                ->add_option("--channel", "Channel number") //
                ->type_name("uint8_t")                      //
                ->default_val(0)                            //
                ->check(utils::ResetValidator(channelOpt));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        channelRequired();
        Super::require(hostOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::channelRequired() {
        Super::require(channelOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getAddress() {
        return SocketAddress(hostOpt->as<std::string>(), channelOpt->as<uint8_t>());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setAddress(const SocketAddress& socketAddress) {
        hostOpt->default_val(socketAddress.address())->clear();
        channelOpt->default_val(socketAddress.channel())->clear();

        Super::initialized();
    }

} // namespace net::rc::config

template class net::rc::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::rc::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::rc::SocketAddress>;
template class net::config::ConfigAddressRemote<net::rc::SocketAddress>;
