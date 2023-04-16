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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H

#include "core/socket/LogicalSocket.h"             // IWYU pragma: export
#include "core/socket/stream/SocketServer.h"       // IWYU pragma: export
#include "core/socket/stream/tls/SocketAcceptor.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename PhysicalServerSocketT, typename ConfigT, typename SocketContextFactoryT>
    using SocketServer = core::socket::stream::SocketServer<core::socket::LogicalSocket<ConfigT>,
                                                            typename PhysicalServerSocketT::SocketAddress,
                                                            core::socket::stream::tls::SocketAcceptor<PhysicalServerSocketT, ConfigT>,
                                                            SocketContextFactoryT>;

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
