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

#ifndef NET_RC_STREAM_TLS_SOCKETCLIENT_H
#define NET_RC_STREAM_TLS_SOCKETCLIENT_H

#include "core/socket/stream/tls/SocketClient.h"         // IWYU pragma: export
#include "net/rc/stream/SocketClient.h"                  // IWYU pragma: export
#include "net/rc/stream/tls/config/ConfigSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::rc::stream::tls {

    template <typename SocketContextFactoryT>
    using SocketClient =
        core::socket::stream::tls::SocketClient<net::rc::stream::SocketClient<net::rc::stream::tls::config::ConfigSocketClient>,
                                                SocketContextFactoryT>;

} // namespace net::rc::stream::tls

#endif // NET_RC_STREAM_TLS_SOCKETCLIENT_H
