/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "net/un/stream/config/ConfigSocketServer.h"

#include "net/config/stream/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream::config {

    ConfigSocketServer::ConfigSocketServer(net::config::ConfigInstance* instance)
        : net::config::stream::ConfigSocketServer<net::un::config::ConfigAddress, net::un::config::ConfigAddressReverse>(instance) {
        net::un::config::ConfigAddress<net::config::ConfigAddressLocal>::sunPathRequired();
    }

    ConfigSocketServer::~ConfigSocketServer() {
    }

} // namespace net::un::stream::config

template class net::config::stream::ConfigSocketServer<net::un::config::ConfigAddress, net::un::config::ConfigAddressReverse>;
