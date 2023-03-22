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

#ifndef NET_UN_STREAM_TLS_SOCKETCLIENT_H
#define NET_UN_STREAM_TLS_SOCKETCLIENT_H

#include "core/socket/stream/tls/SocketClient.h"         // IWYU pragma: export
#include "net/un/stream/SocketClient.h"                  // IWYU pragma: export
#include "net/un/stream/tls/config/ConfigSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::un::stream::tls {

    template <typename SocketContextFactoryT>
    using SocketClient =
        core::socket::stream::tls::SocketClient<net::un::stream::SocketClient<net::un::stream::tls::config::ConfigSocketClient>,
                                                SocketContextFactoryT>;

} // namespace net::un::stream::tls

extern template class net::un::stream::SocketClient<net::un::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::stream::LogicalSocketClient<net::un::stream::PhysicalClientSocket, net::un::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::LogicalSocket<net::un::stream::tls::config::ConfigSocketClient>;

#endif // NET_UN_STREAM_TLS_SOCKETCLIENT_H
