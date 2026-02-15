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

#include "net/un/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"    // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.hpp"   // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.hpp"  // IWYU pragma: keep
#include "net/config/ConfigAddressReverse.hpp" // IWYU pragma: keep
#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.hpp"
#include "utils/Uuid.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"
#include "utils/PreserveErrno.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::init(net::config::ConfigInstance* instance,
                                                const std::string& addressOptionName,
                                                const std::string& addressOptionDescription) {
        Super::init(instance, addressOptionName, addressOptionDescription);
        sunPathOpt = Super::addOption( //
            "--sun-path",
            "Unix domain bind path",
            "filename:FILE",
            std::string('\0' + instance->getInstanceName() + std::to_string(getpid()) + "_" + utils::Uuid::getUuid()));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        SocketAddress* socketAddress = new SocketAddress(sunPathOpt->as<std::string>());

        try {
            socketAddress->init();
        } catch (const core::socket::SocketAddress::BadSocketAddress&) {
            delete socketAddress;
            socketAddress = nullptr;

            throw;
        }

        return socketAddress;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setSunPath(socketAddress.getSunPath());

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

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    void ConfigAddress<ConfigAddressTypeT>::configurable(bool configurable) {
        Super::setConfigurable(sunPathOpt, configurable);
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
template class net::config::ConfigAddressReverse<net::un::SocketAddress>;
template class net::config::ConfigAddressBase<net::un::SocketAddress>;
template class net::un::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::un::config::ConfigAddress<net::config::ConfigAddressRemote>;
template class net::un::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;
