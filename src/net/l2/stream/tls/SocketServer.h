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

#ifndef NET_L2_STREAM_LEGACY_SOCKETSERVER_H
#define NET_L2_STREAM_LEGACY_SOCKETSERVER_H

#include "core/socket/stream/tls/SocketAcceptor.h"       // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnection.h"     // IWYU pragma: export
#include "net/l2/stream/SocketServer.h"                  // IWYU pragma: export
#include "net/l2/stream/tls/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketAcceptor.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnectionFactory.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketReader.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketWriter.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream::tls {

    template <typename SocketContextFactoryT>
    using SocketServer = net::l2::stream::
        SocketServer<core::socket::stream::tls::SocketAcceptor, net::l2::stream::tls::config::ConfigSocketServer, SocketContextFactoryT>;

} // namespace net::l2::stream::tls

extern template class core::socket::Socket<net::l2::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketAcceptor<net::l2::phy::stream::PhysicalSocketServer,
                                                                net::l2::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketConnection<net::l2::phy::stream::PhysicalSocketServer>;
extern template class core::socket::stream::SocketConnectionT<net::l2::phy::stream::PhysicalSocketServer,
                                                              core::socket::stream::tls::SocketReader,
                                                              core::socket::stream::tls::SocketWriter>;

#endif // NET_L2_STREAM_LEGACY_SOCKETSERVER_H
