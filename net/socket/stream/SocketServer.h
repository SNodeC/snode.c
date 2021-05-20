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

#ifndef NET_SOCKET_STREAM_SOCKETSERVERNEW_H
#define NET_SOCKET_STREAM_SOCKETSERVERNEW_H

#include "net/socket/stream/SocketListener.h"
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

    template <typename SocketListenerT>
    class SocketServer {
    public:
        using SocketListener = SocketListenerT;
        using SocketConnection = typename SocketListener::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketServer(const SocketProtocolFactory* socketProtocolFactory,
                     const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onConnected,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : socketProtocolFactory(std::shared_ptr<const SocketProtocolFactory>(socketProtocolFactory))
            , _onConnect(onConnect)
            , _onConnected(onConnected)
            , _onDisconnect(onDisconnect)
            , options(options) {
        }

        SocketServer() = delete;

        virtual ~SocketServer() = default;

        void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int err)>& onError) const {
            errno = 0;

            SocketListener* socketListener = new SocketListener(socketProtocolFactory, _onConnect, _onConnected, _onDisconnect, options);

            socketListener->listen(bindAddress, backlog, onError);
        }

        void onConnect(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect) {
            _onConnect = onConnect;
        }

        void onConnected(const std::function<void(SocketConnection*)>& onConnected) {
            _onConnected = onConnected;
        }

        void onDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            _onDisconnect = onDisconnect;
        }

    protected:
        std::shared_ptr<const SocketProtocolFactory> socketProtocolFactory = nullptr;

    private:
        std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)> _onConnect;
        std::function<void(SocketConnection* socketConnection)> _onConnected;
        std::function<void(SocketConnection* socketConnection)> _onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETSERVERNEW_H
