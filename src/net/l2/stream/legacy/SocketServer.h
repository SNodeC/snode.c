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

#ifndef NET_L2_STREAM_LEGACY_SOCKETSERVER_H
#define NET_L2_STREAM_LEGACY_SOCKETSERVER_H

// IWYU pragma: always_keep

#include "core/socket/stream/legacy/SocketAcceptor.h"
#include "core/socket/stream/legacy/SocketConnection.h"     // IWYU pragma: export
#include "net/l2/stream/SocketServer.h"                     // IWYU pragma: export
#include "net/l2/stream/legacy/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketAcceptor.hpp"
// IWYU pragma: no_include "core/socket/stream/legacy/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream::legacy {

    template <typename SocketContextFactoryT, typename... Args>
    using SocketServer = net::l2::stream::SocketServer<core::socket::stream::legacy::SocketAcceptor,
                                                       net::l2::stream::legacy::config::ConfigSocketServer,
                                                       SocketContextFactoryT,
                                                       Args&&...>;

} // namespace net::l2::stream::legacy

extern template class core::socket::Socket<net::l2::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketAcceptor<net::l2::phy::stream::PhysicalSocketServer,
                                                                   net::l2::stream::legacy::config::ConfigSocketServer>;
extern template class core::socket::stream::legacy::SocketConnection<net::l2::phy::stream::PhysicalSocketServer>;
extern template class core::socket::stream::SocketConnectionT<net::l2::phy::stream::PhysicalSocketServer,
                                                              core::socket::stream::legacy::SocketReader,
                                                              core::socket::stream::legacy::SocketWriter>;

#endif // NET_L2_STREAM_LEGACY_SOCKETSERVER_H
