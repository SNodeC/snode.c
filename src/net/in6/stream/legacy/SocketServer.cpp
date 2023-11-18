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

#include "net/in6/stream/legacy/SocketServer.h"

#include "core/socket/Socket.hpp"                         // IWYU pragma: keep
#include "core/socket/stream/legacy/SocketAcceptor.hpp"   // IWYU pragma: keep
#include "core/socket/stream/legacy/SocketConnection.hpp" // IWYU pragma: keep



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

template class core::socket::Socket<net::in6::stream::legacy::config::ConfigSocketServer>;
template class core::socket::stream::legacy::SocketAcceptor<net::in6::phy::stream::PhysicalSocketServer,
                                                            net::in6::stream::legacy::config::ConfigSocketServer>;
template class core::socket::stream::legacy::SocketConnection<net::in6::phy::stream::PhysicalSocketServer>;
template class core::socket::stream::SocketConnectionT<net::in6::phy::stream::PhysicalSocketServer,
                                                       core::socket::stream::legacy::SocketReader,
                                                       core::socket::stream::legacy::SocketWriter>;
