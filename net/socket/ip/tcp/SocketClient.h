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

#ifndef NET_SOCKET_IP_TCP_SOCKETCLIENT_H
#define NET_SOCKET_IP_TCP_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::ip::tcp {

    template <typename SocketClientSuper>
    class SocketClient : public SocketClientSuper {
    public:
        using SocketClientSuper::SocketClientSuper;

        using SocketConnection = typename SocketClientSuper::SocketConnection;
        using Socket = typename SocketConnection::Socket;

        virtual void connect(const std::map<std::string, std::any>& options,
                             const std::function<void(int err)>& onError,
                             const typename SocketConnection::Socket::SocketAddress& bindAddress =
                                 typename SocketConnection::Socket::SocketAddress()) const {
            std::string host = "";
            unsigned short port = 0;

            for (auto& [name, value] : options) {
                if (name == "host") {
                    host = std::any_cast<const char*>(value);
                } else if (name == "port") {
                    port = std::any_cast<int>(value);
                }
            }
            typename Socket::SocketAddress remoteAddress(host, port);

            SocketClientSuper::connect(remoteAddress, onError, bindAddress);
        }
    };

} // namespace net::socket::ip::tcp

#endif // NET_SOCKET_IP_TCP_SOCKETCLIENT_H
