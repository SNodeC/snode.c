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

#ifndef NET_SOCKET_STREAM_SOCKETCLIENT_H
#define NET_SOCKET_STREAM_SOCKETCLIENT_H

#include "net/socket/stream/SocketConnector.h"
#include "net/socket/stream/SocketProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketProtocolFactoryT, typename SocketConnectorT>
    class SocketClient {
        SocketClient() = delete;

    public:
        using SocketProtocol = SocketProtocolFactoryT;
        using SocketConnector = SocketConnectorT;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketClient(const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onConnected,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : socketProtocol(std::make_shared<SocketProtocol>())
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        virtual ~SocketClient() = default;

        void
        connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int err)>& onError) const {
            SocketConnector* socketConnector = new SocketConnector(socketProtocol, onConnect, onConnected, onDisconnect, options);

            socketConnector->connect(remoteAddress, bindAddress, onError);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(int err)>& onError) const {
            connect(remoteAddress, SocketAddress(), onError);
        }

        std::shared_ptr<SocketProtocol> getSocketProtocol() {
            return socketProtocol;
        }

    protected:
        std::shared_ptr<SocketProtocol> socketProtocol;

    private:
        std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onConnected;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCLIENT_H
