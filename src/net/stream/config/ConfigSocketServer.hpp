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
#include "net/stream/config/ConfigSocketServer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::stream::config {

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    ConfigSocketServer<ConfigAddress>::ConfigSocketServer(net::config::ConfigInstance* instance)
        : ConfigAddress<net::config::ConfigAddressLocal>(instance)
        , net::config::ConfigConnection(instance)
        , net::config::ConfigPhysicalSocket(instance)
        , net::config::ConfigListen(instance)
        , net::config::ConfigCluster(instance) {
        add_socket_option(reuseAddressOpt,
                          "--reuse-address",
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          "Reuse socket address",
                          "bool",
                          "false",
                          CLI::TypeValidator<bool>() & !CLI::Number); // cppcheck-suppress clarifyCondition
    }

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    void ConfigSocketServer<ConfigAddress>::setReuseAddress(bool reuseAddress) {
        if (reuseAddress) {
            addSocketOption(SO_REUSEADDR, SOL_SOCKET);
        } else {
            removeSocketOption(SO_REUSEADDR);
        }

        reuseAddressOpt //
            ->default_val(reuseAddress ? "true" : "false")
            ->take_all()
            ->clear();
    }

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    bool ConfigSocketServer<ConfigAddress>::getReuseAddress() {
        return reuseAddressOpt->as<bool>();
    }

} // namespace net::stream::config
