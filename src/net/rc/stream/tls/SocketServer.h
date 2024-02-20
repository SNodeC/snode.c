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

#ifndef NET_RC_STREAM_TLS_SOCKETSERVER_H
#define NET_RC_STREAM_TLS_SOCKETSERVER_H

// IWYU pragma: always_keep

#include "core/socket/stream/tls/SocketAcceptor.h"
#include "core/socket/stream/tls/SocketConnection.h"     // IWYU pragma: export
#include "net/rc/stream/SocketServer.h"                  // IWYU pragma: export
#include "net/rc/stream/tls/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketAcceptor.hpp"
// IWYU pragma: no_include "core/socket/stream/tls/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: no_include <map>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::rc::stream::tls {

    template <typename SocketContextFactoryT, typename... Args>
    using SocketServer = net::rc::stream::SocketServer<core::socket::stream::tls::SocketAcceptor,
                                                       net::rc::stream::tls::config::ConfigSocketServer,
                                                       SocketContextFactoryT,
                                                       Args...>;

} // namespace net::rc::stream::tls

extern template class core::socket::Socket<net::rc::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketAcceptor<net::rc::phy::stream::PhysicalSocketServer,
                                                                net::rc::stream::tls::config::ConfigSocketServer>;
extern template class core::socket::stream::tls::SocketConnection<net::rc::phy::stream::PhysicalSocketServer>;
extern template class core::socket::stream::SocketConnectionT<net::rc::phy::stream::PhysicalSocketServer,
                                                              core::socket::stream::tls::SocketReader,
                                                              core::socket::stream::tls::SocketWriter>;

#endif // NET_RC_STREAM_TLS_SOCKETSERVER_H
