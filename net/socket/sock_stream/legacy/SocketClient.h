/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCLIENT_H
#define NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnector.h"
#include "socket/sock_stream/SocketClient.h"

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketClient : public socket::stream::SocketClient<legacy::SocketConnector<SocketT>> {
        using socket::stream::SocketClient<legacy::SocketConnector<SocketT>>::SocketClient;
    };

} // namespace net::socket::stream::legacy

#endif // NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCLIENT_H
