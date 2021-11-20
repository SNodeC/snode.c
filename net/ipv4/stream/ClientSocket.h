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

#ifndef NET_IPV4_STREAM_CLIENTSOCKET_H
#define NET_IPV4_STREAM_CLIENTSOCKET_H

#include "net/ipv4/stream/Socket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::ip::stream {

    class ClientSocket {
    public:
        using Socket = net::ip::stream::Socket;
        using SocketAddress = Socket::SocketAddress;

        virtual void
        connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int)>& onError) const = 0;
        virtual void connect(const SocketAddress& remoteAddress, const std::function<void(int)>& onError) const = 0;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname), onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
        }
    };

} // namespace net::ip::stream

#endif // NET_IPV4_STREAM_CLIENTSOCKET_H
