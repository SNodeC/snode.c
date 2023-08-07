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

#include "net/in6/stream/config/ConfigSocketClient.h"

#include "net/stream/config/ConfigSocketClient.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

#include <netinet/in.h>
#include <stdexcept>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream::config {

    ConfigSocketClient::ConfigSocketClient(net::config::ConfigInstance* instance)
        : net::stream::config::ConfigSocketClient<net::in6::config::ConfigAddress>(instance) {
        net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>::hostRequired();
        net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>::portRequired();

        net::config::ConfigPhysicalSocket::add_socket_option(iPv6OnlyOpt,
                                                             "--ipv6-only",
                                                             IPPROTO_IPV6,
                                                             IPV6_V6ONLY,
                                                             "Turn of IPv6 dual stack mode",
                                                             "bool",
                                                             "false",
                                                             CLI::IsMember({"true", "false"}));

        net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>::setIpv6OnlyOpt(iPv6OnlyOpt);
        net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>::setIpv6OnlyOpt(iPv6OnlyOpt);
    }

    ConfigSocketClient::~ConfigSocketClient() {
    }

    void ConfigSocketClient::setIPv6Only(bool iPv6Only) {
        if (iPv6Only) {
            addSocketOption(IPPROTO_IPV6, IPV6_V6ONLY, 1);
        } else {
            removeSocketOption(IPV6_V6ONLY);
        }

        iPv6OnlyOpt //
            ->default_val(iPv6Only ? "true" : "false")
            ->take_all()
            ->clear();
    }

    bool ConfigSocketClient::getIPv6Only() {
        return iPv6OnlyOpt->as<bool>();
    }

} // namespace net::in6::stream::config

template class net::stream::config::ConfigSocketClient<net::in6::config::ConfigAddress>;
