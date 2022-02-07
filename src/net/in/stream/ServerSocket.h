/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef NET_IN_STREAM_SERVERSOCKET_H
#define NET_IN_STREAM_SERVERSOCKET_H

#include "core/socket/ServerSocket.h" // IWYU pragma: export
#include "net/in/stream/Socket.h"     // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <functional>
#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    class ServerSocket : public core::socket::ServerSocket<net::in::stream::Socket> {
        using Super = core::socket::ServerSocket<net::in::stream::Socket>;

    public:
        using core::socket::ServerSocket<net::in::stream::Socket>::listen;

        void listen(uint16_t port, int backlog, const std::function<void(const SocketAddress&, int)>& onError);

        void listen(const std::string& ipOrHostname, int backlog, const std::function<void(const SocketAddress&, int)>& onError);

        void
        listen(const std::string& ipOrHostname, uint16_t port, int backlog, const std::function<void(const SocketAddress&, int)>& onError);
    };

} // namespace net::in::stream

#endif // NET_IN_STREAM_SERVERSOCKET_H
