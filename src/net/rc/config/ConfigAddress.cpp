/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "net/config/ConfigAddressBase.hpp"    // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.hpp"   // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.hpp"  // IWYU pragma: keep
#include "net/config/ConfigAddressReverse.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance,
                                                    const std::string& addressOptionName,
                                                    const std::string& addressOptionDescription)
        : Super(instance, addressOptionName, addressOptionDescription) {
        Super::add_option(btAddressOpt, //
                          "--host",
                          "Bluetooth address",
                          "xx:xx:xx:xx:xx:xx",
                          "00:00:00:00:00:00",
                          CLI::TypeValidator<std::string>());
        Super::add_option(channelOpt, //
                          "--channel",
                          "Channel number",
                          "channel");
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        return &(new SocketAddress(btAddressOpt->as<std::string>(), channelOpt->as<uint8_t>()))->init();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setBtAddress(socketAddress.getBtAddress());
        setChannel(socketAddress.getChannel());

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setBtAddress(const std::string& btAddress) {
        const utils::PreserveErrno preserveErrno;

        btAddressOpt //
            ->default_val(btAddress)
            ->clear();
        Super::required(btAddressOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getBtAddress() const {
        return btAddressOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setChannel(uint8_t channel) {
        const utils::PreserveErrno preserveErrno;

        channelOpt //
            ->default_val<int>(channel)
            ->clear();
        Super::required(channelOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint8_t ConfigAddress<ConfigAddressType>::getChannel() const {
        return channelOpt->as<uint8_t>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setBtAddressRequired(bool required) {
        Super::required(btAddressOpt, required);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setChannelRequired(bool required) {
        Super::required(channelOpt, required);

        return *this;
    }

} // namespace net::rc::config

template class net::config::ConfigAddress<net::rc::SocketAddress>;
template class net::config::ConfigAddressLocal<net::rc::SocketAddress>;
template class net::config::ConfigAddressRemote<net::rc::SocketAddress>;
template class net::config::ConfigAddressReverse<net::rc::SocketAddress>;
template class net::config::ConfigAddressBase<net::rc::SocketAddress>;
template class net::rc::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::rc::config::ConfigAddress<net::config::ConfigAddressRemote>;
template class net::rc::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;
