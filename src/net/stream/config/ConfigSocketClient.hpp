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

#include "net/config/ConfigSection.hpp"
#include "net/stream/config/ConfigSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef CONNECT_TIMEOUT
#define CONNECT_TIMEOUT 10
#endif

namespace net::stream::config {

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    ConfigSocketClient<ConfigAddress>::ConfigSocketClient(net::config::ConfigInstance* instance)
        : ConfigAddress<net::config::ConfigAddressRemote>(instance)
        , ConfigAddress<net::config::ConfigAddressLocal>(instance)
        , net::config::ConfigConnection(instance)
        , net::config::ConfigPhysicalSocket(instance) {
        net::config::ConfigPhysicalSocket::add_option(connectTimeoutOpt, //
                                                      "--connect-timeout",
                                                      "Connect timeout",
                                                      "timeout",
                                                      CONNECT_TIMEOUT,
                                                      CLI::NonNegativeNumber);
    }

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    void ConfigSocketClient<ConfigAddress>::setConnectTimeout(const utils::Timeval& connectTimeout) {
        connectTimeoutOpt //
            ->default_val(connectTimeout)
            ->clear();
    }

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    utils::Timeval ConfigSocketClient<ConfigAddress>::getConnectTimeout() const {
        double connectTimeout = connectTimeoutOpt->as<double>();
        return utils::Timeval(connectTimeout);
    }

} // namespace net::stream::config
