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

#include "net/rc/stream/legacy/SocketServer.h"

#include "core/socket/LogicalSocket.hpp"                  // IWYU pragma: keep
#include "core/socket/stream/SocketAcceptor.hpp"          // IWYU pragma: keep
#include "core/socket/stream/SocketConnection.hpp"        // IWYU pragma: keep
#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: keep
#include "core/socket/stream/legacy/SocketAcceptor.hpp"   // IWYU pragma: keep
#include "core/socket/stream/legacy/SocketConnection.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

template class core::socket::LogicalSocket<net::rc::stream::legacy::config::ConfigSocketServer>;
template class core::socket::stream::legacy::SocketAcceptor<net::rc::stream::PhysicalServerSocket,
                                                            net::rc::stream::legacy::config::ConfigSocketServer>;
template class core::socket::stream::legacy::SocketConnection<net::rc::stream::PhysicalServerSocket>;
template class core::socket::stream::SocketAcceptor<net::rc::stream::PhysicalServerSocket,
                                                    net::rc::stream::legacy::config::ConfigSocketServer,
                                                    core::socket::stream::legacy::SocketConnection>;
template class core::socket::stream::SocketConnectionT<net::rc::stream::PhysicalServerSocket,
                                                       core::socket::stream::legacy::SocketReader,
                                                       core::socket::stream::legacy::SocketWriter>;
template class core::socket::stream::SocketConnectionFactory<
    net::rc::stream::PhysicalServerSocket,
    net::rc::stream::legacy::config::ConfigSocketServer,
    core::socket::stream::legacy::SocketConnection<net::rc::stream::PhysicalServerSocket>>;
