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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/ResetValidator.h"

#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress() {
        if (!net::config::ConfigInstance::getInstanceName().empty()) {
            sunPathOpt = //
                ConfigAddressType::addressSc
                    ->add_option("--path", sunPath, "Unix domain socket")                                                              //
                    ->type_name("sun_path:FILE")                                                                                       //
                    ->default_val(std::string('\0' + net::config::ConfigInstance::getInstanceName() + "_" + std::to_string(getpid()))) //
                    ->check(utils::ResetValidator(sunPathOpt));
        }

        ConfigAddressType::address.setSunPath(
            std::string('\0' + net::config::ConfigInstance::getInstanceName() + "_" + std::to_string(getpid())));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::required() {
        sunPathRequired();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::sunPathRequired() {
        ConfigAddressType::require(sunPathOpt);
        ConfigAddressType::address.setSunPath("");
        sunPath = "";
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::updateFromCommandLine() {
        if (sunPathOpt->count() > 0) {
            ConfigAddressType::address.setSunPath(sunPath);
        }
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::addressDefaultsFromCurrent() {
        sunPathOpt //
            ->default_val(ConfigAddressType::address.address())
            ->required(false)
            ->clear();
    }

} // namespace net::un::config

template class net::un::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::un::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::un::SocketAddress>;
template class net::config::ConfigAddressRemote<net::un::SocketAddress>;
