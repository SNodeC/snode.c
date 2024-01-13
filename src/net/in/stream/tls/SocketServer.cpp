/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "net/in/stream/tls/SocketServer.h"

#include "core/socket/Socket.hpp"                      // IWYU pragma: keep
#include "core/socket/stream/SocketConnection.hpp"     // IWYU pragma: keep
#include "core/socket/stream/tls/SocketAcceptor.hpp"   // IWYU pragma: keep
#include "core/socket/stream/tls/SocketConnection.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

template class core::socket::Socket<net::in::stream::tls::config::ConfigSocketServer>;
template class core::socket::stream::tls::SocketAcceptor<net::in::phy::stream::PhysicalSocketServer,
                                                         net::in::stream::tls::config::ConfigSocketServer>;
template class core::socket::stream::tls::SocketConnection<net::in::phy::stream::PhysicalSocketServer>;
template class core::socket::stream::SocketConnectionT<net::in::phy::stream::PhysicalSocketServer,
                                                       core::socket::stream::tls::SocketReader,
                                                       core::socket::stream::tls::SocketWriter>;
