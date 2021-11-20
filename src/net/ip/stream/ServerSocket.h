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

#ifndef NET_IPV4_STREAM_SERVERSOCKET_H
#define NET_IPV4_STREAM_SERVERSOCKET_H

#include "net/ip/stream/Socket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::ip::stream {

    class ServerSocket {
    public:
        using Socket = net::ip::stream::Socket;
        using SocketAddress = Socket::SocketAddress;

        virtual void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) const = 0;

        void listen(uint16_t port, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(port), backlog, onError);
        }

        void listen(const std::string& ipOrHostname, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(ipOrHostname), backlog, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(ipOrHostname, port), backlog, onError);
        }
    };

} // namespace net::ip::stream

#endif // NET_IPV4_STREAM_SERVERSOCKET_H
