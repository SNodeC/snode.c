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

#include "net/in/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/PreserveErrno.h"
#include "utils/ResetValidator.h"

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        hostOpt = //
            Super::addressSc
                ->add_option("--host", "Host name or IPv4 address") //
                ->type_name("hostname|IPv4")                        //
                ->default_val("0.0.0.0")                            //
                ->check(utils::ResetValidator(hostOpt));

        portOpt = //
            Super::addressSc
                ->add_option("--port", "Port number") //
                ->type_name("uint16_t")               //
                ->default_val(0)                      //
                ->check(utils::ResetValidator(portOpt));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        portRequired();
        Super::require(hostOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::portRequired() {
        Super::require(portOpt);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getAddress() const {
        utils::PreserveErrno preserveErrno;

        return SocketAddress(hostOpt->as<std::string>(), portOpt->as<uint16_t>());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setAddress(const SocketAddress& socketAddress) {
        utils::PreserveErrno preserveErrno;

        hostOpt->default_val(socketAddress.host())->required(false)->clear();
        portOpt->default_val(socketAddress.port())->required(false)->clear();
    }

} // namespace net::in::config

template class net::in::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::in::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::in::SocketAddress>;
template class net::config::ConfigAddressRemote<net::in::SocketAddress>;
