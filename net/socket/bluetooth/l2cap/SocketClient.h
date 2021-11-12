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

#ifndef NET_SOCKET_BLUETOOTH_L2CAP_SOCKETCLIENT_H
#define NET_SOCKET_BLUETOOTH_L2CAP_SOCKETCLIENT_H

#include "net/socket/bluetooth/l2cap/Socket.h"
#include "net/socket/stream/legacy/SocketClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::bluetooth::l2cap {

    template <typename SocketContextFactoryT>
    class SocketClient : public net::socket::stream::legacy::SocketClient<l2cap::Socket, SocketContextFactoryT> {
        using net::socket::stream::legacy::SocketClient<l2cap::Socket, SocketContextFactoryT>::SocketClient;

    public:
        using SocketAddress = typename net::socket::stream::legacy::SocketClient<l2cap::Socket, SocketContextFactoryT>::SocketAddress;

        using net::socket::stream::legacy::SocketClient<l2cap::Socket, SocketContextFactoryT>::connect;

        void connect(const std::string& address, uint16_t psm, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, psm), onError);
        }

        void connect(const std::string& address, uint16_t psm, const std::string& bindAddress, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, psm), SocketAddress(bindAddress), onError);
        }

        void connect(const std::string& address,
                     uint16_t psm,
                     const std::string& bindAddress,
                     uint16_t bindPsm,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(address, psm), SocketAddress(bindAddress, bindPsm), onError);
        }
    };

} // namespace net::socket::bluetooth::l2cap

#endif // NET_SOCKET_BLUETOOTH_L2CAP_SOCKETCLIENT_H
