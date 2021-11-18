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

#ifndef NET_SOCKET_IP_TCP_SOCKETCLIENT_H
#define NET_SOCKET_IP_TCP_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::socket::ip::transport::tcp {

    template <template <typename SocketT, typename SocketContextFactoryT> typename SocketClientBaseT,
              typename SocketT,
              typename SocketContextFactoryT>
    class SocketClient : public SocketClientBaseT<SocketT, SocketContextFactoryT> {
        using SocketClientBase = SocketClientBaseT<SocketT, SocketContextFactoryT>;

        using SocketClientBase::SocketClientBase;

    public:
        using SocketAddress = typename SocketClientBase::SocketAddress;

        using SocketClientBase::connect;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
            SocketClientBase::connect(SocketAddress(ipOrHostname, port), onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     const std::function<void(int)>& onError) {
            SocketClientBase::connect(SocketAddress(ipOrHostname, port), bindIpOrHostname, onError);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(int)>& onError) {
            SocketClientBase::connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
        }
    };

} // namespace net::socket::ip::transport::tcp

#endif // NET_SOCKET_IP_TCP_SOCKETCLIENT_H
