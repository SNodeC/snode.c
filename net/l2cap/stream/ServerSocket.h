/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_L2CAP_STREAM_SERVERSOCKET_H
#define NET_L2CAP_STREAM_SERVERSOCKET_H

#include "net/l2cap/stream/Socket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    class ServerSocket {
    public:
        using Socket = net::l2::stream::Socket;
        using SocketAddress = Socket::SocketAddress;

        virtual void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) const = 0;

        void listen(uint16_t psm, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(psm), backlog, onError);
        }

        void listen(const std::string& address, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address), backlog, onError);
        }

        void listen(const std::string& address, uint16_t psm, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address, psm), backlog, onError);
        }
    };

} // namespace net::l2::stream

#endif // NET_L2CAP_STREAM_SERVERSOCKET_H
