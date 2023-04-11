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

#ifndef NET_IPV4_STREAM_LEGACY_SOCKETCLIENT_H
#define NET_IPV4_STREAM_LEGACY_SOCKETCLIENT_H

#include "core/socket/stream/legacy/SocketClient.h"         // IWYU pragma: export
#include "net/in/stream/SocketClient.h"                     // IWYU pragma: export
#include "net/in/stream/legacy/config/ConfigSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in::stream::legacy {

    template <typename SocketContextFactoryT>
    using SocketClient =
        net::in::stream::SocketClient<core::socket::stream::legacy::SocketClient<net::in::stream::PhysicalClientSocket,
                                                                                 net::in::stream::legacy::config::ConfigSocketClient,
                                                                                 SocketContextFactoryT>>;

} // namespace net::in::stream::legacy

extern template class core::socket::LogicalSocket<net::in::stream::legacy::config::ConfigSocketClient>;

#endif // NET_IPV4_STREAM_LEGACY_SOCKETCLIENT_H
