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

#ifndef NET_IN_STREAM_LEGACY_SOCKETSERVER_H
#define NET_IN_STREAM_LEGACY_SOCKETSERVER_H

#include "core/socket/stream/legacy/SocketConnection.h"     // IWYU pragma: export
#include "core/socket/stream/legacy/SocketServer.h"         // IWYU pragma: export
#include "net/in/stream/SocketServer.h"                     // IWYU pragma: export
#include "net/in/stream/legacy/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketAcceptor.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnectionFactory.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketReader.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in::stream::legacy {

    template <typename SocketContextFactoryT>
    using SocketServer =
        net::in::stream::SocketServer<core::socket::stream::legacy::SocketServer<net::in::stream::PhysicalServerSocket,
                                                                                 net::in::stream::legacy::config::ConfigSocketServer,
                                                                                 SocketContextFactoryT>>;

} // namespace net::in::stream::legacy

extern template class core::socket::LogicalSocket<net::in::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketAcceptor<net::in::stream::PhysicalServerSocket,
                                                                   net::in::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketConnection<net::in::stream::PhysicalServerSocket>;
extern template class core::socket::stream::SocketConnectionT<net::in::stream::PhysicalServerSocket,
                                                              core::socket::stream::legacy::SocketReader,
                                                              core::socket::stream::legacy::SocketWriter>;

#endif // NET_IN_STREAM_LEGACY_SOCKETSERVER_H
