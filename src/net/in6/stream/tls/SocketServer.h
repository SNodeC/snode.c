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

#ifndef NET_IN6_STREAM_TLS_SOCKETSERVER_H
#define NET_IN6_STREAM_TLS_SOCKETSERVER_H

#include "core/socket/stream/tls/SocketAcceptor.h"        // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnection.h"      // IWYU pragma: export
#include "net/in6/stream/SocketServer.h"                  // IWYU pragma: export
#include "net/in6/stream/tls/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketAcceptor.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnectionFactory.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketReader.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketWriter.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: no_include <map>
// IWYU pragma: no_include <set>
// IWYU pragma: no_include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in6::stream::tls {

    template <typename SocketContextFactoryT, typename... Args>
    using SocketServer = net::in6::stream::SocketServer<core::socket::stream::tls::SocketAcceptor,
                                                        net::in6::stream::tls::config::ConfigSocketServer,
                                                        SocketContextFactoryT,
                                                        Args&&...>;

} // namespace net::in6::stream::tls

extern template class core::socket::Socket<net::in6::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketAcceptor<net::in6::phy::stream::PhysicalSocketServer,
                                                                net::in6::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketConnection<net::in6::phy::stream::PhysicalSocketServer>;
extern template class core::socket::stream::SocketConnectionT<net::in6::phy::stream::PhysicalSocketServer,
                                                              core::socket::stream::tls::SocketReader,
                                                              core::socket::stream::tls::SocketWriter>;

#endif // NET_IN6_STREAM_TLS_SOCKETSERVER_H
