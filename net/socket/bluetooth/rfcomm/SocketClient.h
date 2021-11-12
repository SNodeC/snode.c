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

#ifndef NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETCLIENT_H
#define NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <cstdint>
#include <functional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::socket::bluetooth::rfcomm {

    template <typename ConcreteSocketClient>
    class SocketClient : public ConcreteSocketClient {
        using  ConcreteSocketClient::ConcreteSocketClient;

    public:
        using SocketAddress = typename ConcreteSocketClient::SocketAddress;

        using ConcreteSocketClient::connect;

        void connect(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), onError);
        }

        void connect(const std::string& address, uint8_t channel, const std::string& bindAddress, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), SocketAddress(bindAddress), onError);
        }

        void connect(const std::string& address,
                     uint8_t channel,
                     const std::string& bindAddress,
                     uint8_t bindChannel,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), SocketAddress(bindAddress, bindChannel), onError);
        }
    };

} // namespace net::socket::bluetooth::rfcomm

#endif // NET_SOCKET_BLUETOOTH_RFCOMM_SOCKETCLIENT_H
