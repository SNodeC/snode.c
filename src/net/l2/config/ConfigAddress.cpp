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

#include "net/l2/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <cstdint>
#include <limits>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance)
        : Super(instance) {
        Super::add_option(hostOpt, //
                          "--host",
                          "Bluetooth address",
                          "xx:xx:xx:xx:xx:xx",
                          "00:00:00:00:00:00",
                          CLI::TypeValidator<std::string>());
        Super::add_option(psmOpt, //
                          "--psm",
                          "Protocol service multiplexer",
                          "psm",
                          0,
                          CLI::Range(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::hostRequired() {
        Super::required(hostOpt, true);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::psmRequired() {
        Super::required(psmOpt, true);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getAddress() const {
        utils::PreserveErrno preserveErrno;

        return SocketAddress(hostOpt->as<std::string>(), psmOpt->as<uint16_t>());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setAddress(const SocketAddress& socketAddress) {
        setBtAddress(socketAddress.address());
        setPsm(socketAddress.psm());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getBtAddress() {
        return hostOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setBtAddress(const std::string& btAddress) {
        utils::PreserveErrno preserveErrno;

        hostOpt //
            ->default_val(btAddress);
        Super::required(hostOpt, false);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint16_t ConfigAddress<ConfigAddressType>::getPsm() {
        return psmOpt->as<uint16_t>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setPsm(uint16_t psm) {
        utils::PreserveErrno preserveErrno;

        psmOpt //
            ->default_val(psm);
        Super::required(psmOpt, false);
    }

} // namespace net::l2::config

template class net::l2::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::l2::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::l2::SocketAddress>;
template class net::config::ConfigAddressRemote<net::l2::SocketAddress>;
