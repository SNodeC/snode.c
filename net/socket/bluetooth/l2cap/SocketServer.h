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

#ifndef NET_SOCKET_BLUETOOTH_L2CAP_SOCKETSERVER_H
#define NET_SOCKET_BLUETOOTH_L2CAP_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/sock_stream/legacy/SocketServer.h"
#include "socket/bluetooth/l2cap/Socket.h"

namespace net::socket::bluetooth::l2cap {

    class SocketServer : public stream::legacy::SocketServer<l2cap::Socket> {
    public:
        using stream::legacy::SocketServer<l2cap::Socket>::SocketServer;

        using SocketConnection = typename stream::legacy::SocketServer<l2cap::Socket>::SocketConnection;
        using SocketAddress = typename Socket::SocketAddress;

        void listen(uint16_t psm, int backlog, const std::function<void(int err)>& onError) const {
            stream::legacy::SocketServer<Socket>::listen(SocketAddress(psm), backlog, onError);
        }

        void listen(const std::string& btAddress, uint16_t psm, int backlog, const std::function<void(int err)>& onError) const {
            stream::legacy::SocketServer<Socket>::listen(SocketAddress(btAddress, psm), backlog, onError);
        }
    };

} // namespace net::socket::bluetooth::l2cap

#endif // NET_SOCKET_BLUETOOTH_L2CAP_SOCKETSERVER_H
