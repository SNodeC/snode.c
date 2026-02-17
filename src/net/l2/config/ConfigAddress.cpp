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

#include "net/l2/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"    // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.hpp"   // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.hpp"  // IWYU pragma: keep
#include "net/config/ConfigAddressReverse.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddressReverse<ConfigAddressType>::ConfigAddressReverse(net::config::ConfigInstance* instance,
                                                                  [[maybe_unused]] const std::string& addressOptionName,
                                                                  [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::l2::SocketAddress>(this) {
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance,
                                                    [[maybe_unused]] const std::string& addressOptionName,
                                                    [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::l2::SocketAddress>(this) {
        btAddressOpt = addOption( //
            "--host",
            "Bluetooth address",
            "xx:xx:xx:xx:xx:xx",
            "00:00:00:00:00:00",
            CLI::TypeValidator<std::string>());
        psmOpt = addOption( //
            "--psm",
            "Protocol service multiplexer",
            "psm");
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        SocketAddress* socketAddress = new SocketAddress(btAddressOpt->as<std::string>(), psmOpt->as<uint16_t>());
        socketAddress->init();

        return socketAddress;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setBtAddress(socketAddress.getBtAddress());
        setPsm(socketAddress.getPsm());

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setBtAddress(const std::string& btAddress) {
        const utils::PreserveErrno preserveErrno;

        btAddressOpt //
            ->default_val(btAddress)
            ->clear();
        this->required(btAddressOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getBtAddress() const {
        return btAddressOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPsm(uint16_t psm) {
        const utils::PreserveErrno preserveErrno;

        psmOpt //
            ->default_val(psm)
            ->clear();
        this->required(psmOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint16_t ConfigAddress<ConfigAddressType>::getPsm() const {
        return psmOpt->as<uint16_t>();
    }

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    void ConfigAddress<ConfigAddressTypeT>::configurable(bool configurable) {
        this->setConfigurable(btAddressOpt, configurable);
        this->setConfigurable(psmOpt, configurable);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setBtAddressRequired(bool required) {
        this->required(btAddressOpt, required);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPsmRequired(bool required) {
        this->required(psmOpt, required);

        return *this;
    }

} // namespace net::l2::config

template class net::config::ConfigAddress<net::l2::SocketAddress>;
template class net::config::ConfigAddressLocal<net::l2::SocketAddress>;
template class net::config::ConfigAddressRemote<net::l2::SocketAddress>;
template class net::config::ConfigAddressReverse<net::l2::SocketAddress>;
template class net::config::ConfigAddressBase<net::l2::SocketAddress>;
template class net::l2::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::l2::config::ConfigAddress<net::config::ConfigAddressRemote>;
template class net::l2::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;
