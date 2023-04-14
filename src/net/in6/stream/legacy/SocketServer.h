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

#ifndef NET_IN6_STREAM_LEGACY_SOCKETSERVER_H
#define NET_IN6_STREAM_LEGACY_SOCKETSERVER_H

#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/legacy/SocketAcceptor.h"
#include "core/socket/stream/legacy/SocketConnection.h"
#include "core/socket/stream/legacy/SocketServer.h"          // IWYU pragma: export
#include "net/in6/stream/SocketServer.h"                     // IWYU pragma: export
#include "net/in6/stream/legacy/config/ConfigSocketServer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in6::stream::legacy {

    template <typename SocketContextFactoryT>
    using SocketServer =
        net::in6::stream::SocketServer<core::socket::stream::legacy::SocketServer<net::in6::stream::PhysicalServerSocket,
                                                                                  net::in6::stream::legacy::config::ConfigSocketServer,
                                                                                  SocketContextFactoryT>>;

} // namespace net::in6::stream::legacy

extern template class core::socket::LogicalSocket<net::in6::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketAcceptor<net::in6::stream::PhysicalServerSocket,
                                                                   net::in6::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketConnection<net::in6::stream::PhysicalServerSocket>;
extern template class core::socket::stream::SocketAcceptor<net::in6::stream::PhysicalServerSocket,
                                                           net::in6::stream::legacy::config::ConfigSocketServer,
                                                           core::socket::stream::legacy::SocketConnection>;

#endif // NET_IN6_STREAM_LEGACY_SOCKETSERVER_H
