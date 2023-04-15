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

#include "net/l2/stream/tls/SocketClient.h"

#include "core/socket/LogicalSocket.hpp"                  // IWYU pragma: keep
#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: keep
#include "core/socket/stream/tls/SocketConnection.hpp"    // IWYU pragma: keep
#include "core/socket/stream/tls/SocketConnector.hpp"     // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

template class core::socket::LogicalSocket<net::l2::stream::tls::config::ConfigSocketClient>;
template class core::socket::stream::tls::SocketConnector<net::l2::stream::PhysicalClientSocket,
                                                          net::l2::stream::tls::config::ConfigSocketClient>;
template class core::socket::stream::tls::SocketConnection<net::l2::stream::PhysicalClientSocket>;
template class core::socket::stream::SocketConnector<net::l2::stream::PhysicalClientSocket,
                                                     net::l2::stream::tls::config::ConfigSocketClient,
                                                     core::socket::stream::tls::SocketConnection>;
template class core::socket::stream::SocketConnectionT<net::l2::stream::PhysicalClientSocket,
                                                       core::socket::stream::tls::SocketReader,
                                                       core::socket::stream::tls::SocketWriter>;
template class core::socket::stream::SocketConnectionFactory<
    net::l2::stream::PhysicalClientSocket,
    net::l2::stream::tls::config::ConfigSocketClient,
    core::socket::stream::tls::SocketConnection<net::l2::stream::PhysicalClientSocket>>;
template class core::socket::stream::tls::SocketReader<net::l2::stream::PhysicalClientSocket>;
template class core::socket::stream::SocketReader<net::l2::stream::PhysicalClientSocket>;
