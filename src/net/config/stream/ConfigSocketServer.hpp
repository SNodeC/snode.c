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

// #include "net/config/ConfigSection.hpp"
#include "net/config/stream/ConfigSocketServer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream {

    template <template <template <typename SocketAddress> typename ConfigAddressType> typename ConfigAddress>
    ConfigSocketServer<ConfigAddress>::ConfigSocketServer(net::config::ConfigInstance* instance)
        : ConfigAddress<net::config::ConfigAddressLocal>(instance)
        , net::config::ConfigConnection(instance)
        , net::config::ConfigPhysicalSocketServer(instance)
        , net::config::ConfigListen(instance) {
    }

} // namespace net::config::stream
