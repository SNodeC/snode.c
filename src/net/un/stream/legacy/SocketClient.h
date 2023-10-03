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

#ifndef NET_UN_STREAM_LEGACY_SOCKETCLIENT_H
#define NET_UN_STREAM_LEGACY_SOCKETCLIENT_H

#include "core/socket/stream/legacy/SocketConnection.h"     // IWYU pragma: export
#include "core/socket/stream/legacy/SocketConnector.h"      // IWYU pragma: export
#include "net/un/stream/SocketClient.h"                     // IWYU pragma: export
#include "net/un/stream/legacy/config/ConfigSocketClient.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketConnector.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnectionFactory.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketReader.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketWriter.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::un::stream::legacy {

    template <typename SocketContextFactoryT>
    using SocketClient = net::un::stream::SocketClient<core::socket::stream::legacy::SocketConnector,
                                                       net::un::stream::legacy::config::ConfigSocketClient,
                                                       SocketContextFactoryT>;

} // namespace net::un::stream::legacy

extern template class core::socket::Socket<net::un::stream::legacy::config::ConfigSocketClient>;
extern template class core::socket::stream::legacy::SocketConnector<net::un::stream::PhysicalSocketClient,
                                                                    net::un::stream::legacy::config::ConfigSocketClient>;
extern template class core::socket::stream::legacy::SocketConnection<net::un::stream::PhysicalSocketClient>;
extern template class core::socket::stream::SocketConnectionT<net::un::stream::PhysicalSocketClient,
                                                              core::socket::stream::legacy::SocketReader,
                                                              core::socket::stream::legacy::SocketWriter>;

#endif // NET_UN_STREAM_LEGACY_SOCKETCLIENT_H
