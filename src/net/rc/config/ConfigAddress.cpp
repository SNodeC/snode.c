/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/rc/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"    // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.hpp"   // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.hpp"  // IWYU pragma: keep
#include "net/config/ConfigAddressReverse.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <regex>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddressReverse<ConfigAddressType>::ConfigAddressReverse(net::config::ConfigInstance* instance,
                                                                  [[maybe_unused]] const std::string& addressOptionName,
                                                                  [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::rc::SocketAddress>(this) {
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance,
                                                    [[maybe_unused]] const std::string& addressOptionName,
                                                    [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::rc::SocketAddress>(this) {
        btAddressOpt = addOption( //
            "--host",
            "Bluetooth address (format 01:23:45:67:89:AB)",
            "address",
            "00:00:00:00:00:00",
            CLI::Validator(
                [](std::string& s) -> std::string {
                    // Classic 48-bit Bluetooth/MAC style: "01:23:45:67:89:AB"
                    static const std::regex re(R"(^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$)");

                    if (std::regex_match(s, re)) {
                        return {}; // OK
                    }

                    return "Invalid Bluetooth address. Expected format like \"01:23:45:67:89:AB\" "
                           "(6 hex octets separated by ':').";
                },
                "BT_ADDR")
                .name("BT_ADDR"));

        channelOpt = addOption( //
            "--channel",
            "Channel number",
            "channel",
            "1",
            CLI::Range(static_cast<uint16_t>(1), static_cast<uint16_t>(30)));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        SocketAddress* socketAddress = new SocketAddress(btAddressOpt->as<std::string>(), channelOpt->as<uint8_t>());
        socketAddress->init();

        return socketAddress;
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

        setDefaultValue(btAddressOpt, btAddress);
        this->required(btAddressOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getBtAddress() const {
        return btAddressOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setChannel(uint8_t channel) {
        const utils::PreserveErrno preserveErrno;

        setDefaultValue<int>(channelOpt, channel);
        this->required(channelOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint8_t ConfigAddress<ConfigAddressType>::getChannel() const {
        return channelOpt->as<uint8_t>();
    }

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    void ConfigAddress<ConfigAddressTypeT>::configurable(bool configurable) {
        this->setConfigurable(btAddressOpt, configurable);
        this->setConfigurable(channelOpt, configurable);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setBtAddressRequired(bool required) {
        this->required(btAddressOpt, required);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setChannelRequired(bool required) {
        this->required(channelOpt, required);

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
