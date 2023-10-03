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

#include "net/in6/stream/PhysicalSocketServer.h"

#include "net/in6/stream/PhysicalSocket.hpp"
#include "net/phy/stream/PhysicalSocketServer.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream {

    PhysicalSocketServer::~PhysicalSocketServer() {
    }

} // namespace net::in6::stream

template class net::phy::stream::PhysicalSocketServer<net::in6::SocketAddress>;
template class net::in6::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;
template class net::in6::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;
