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

#ifndef NET_L2_STREAM_TLS_SOCKETCLIENT_H
#define NET_L2_STREAM_TLS_SOCKETCLIENT_H

#include "core/socket/stream/tls/SocketConnection.h"
#include "core/socket/stream/tls/SocketConnector.h"
#include "net/l2/stream/SocketClient.h"                  // IWYU pragma: export
#include "net/l2/stream/tls/config/ConfigSocketClient.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketConnector.hpp"
// IWYU pragma: no_include "core/socket/stream/tls/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnectionFactory.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketReader.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketWriter.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream::tls {

    template <typename SocketContextFactoryT, typename... Args>
    using SocketClient = net::l2::stream::SocketClient<core::socket::stream::tls::SocketConnector,
                                                       net::l2::stream::tls::config::ConfigSocketClient,
                                                       SocketContextFactoryT,
                                                       Args&&...>;

} // namespace net::l2::stream::tls

extern template class core::socket::Socket<net::l2::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::stream::tls::SocketConnector<net::l2::phy::stream::PhysicalSocketClient,
                                                                 net::l2::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::stream::tls::SocketConnection<net::l2::phy::stream::PhysicalSocketClient>;
extern template class core::socket::stream::SocketConnectionT<net::l2::phy::stream::PhysicalSocketClient,
                                                              core::socket::stream::tls::SocketReader,
                                                              core::socket::stream::tls::SocketWriter>;

#endif // NET_L2_STREAM_TLS_SOCKETCLIENT_H
