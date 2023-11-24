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

#include "net/un/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"
#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.hpp"
#include "utils/Uuid.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <string>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance)
        : Super(instance) {
        Super::add_option(sunPathOpt, //
                          "--path",
                          "Unix domain socket file",
                          "filename:FILE",
                          std::string('\0' + instance->getInstanceName() + std::to_string(getpid()) + "_" + utils::Uuid::getUuid()));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        return new SocketAddress(sunPathOpt->as<std::string>());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setSunPath(socketAddress.getAddress());

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSunPath(const std::string& sunPath) {
        const utils::PreserveErrno preserveErrno;

        sunPathOpt //
            ->default_val(sunPath)
            ->clear();
        Super::required(sunPathOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getSunPath() const {
        return sunPathOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::sunPathRequired(bool required) {
        Super::required(sunPathOpt, required);

        return *this;
    }

} // namespace net::un::config

template class net::config::ConfigAddress<net::un::SocketAddress>;
template class net::config::ConfigAddressLocal<net::un::SocketAddress>;
template class net::config::ConfigAddressRemote<net::un::SocketAddress>;
template class net::un::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::un::config::ConfigAddress<net::config::ConfigAddressRemote>;
