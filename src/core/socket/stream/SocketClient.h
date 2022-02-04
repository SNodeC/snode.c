/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_SOCKETCLIENT_H
#define CORE_SOCKET_STREAM_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any> // IWYU pragma: export
#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <map>        // IWYU pragma: export
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename ClientSocketT, typename SocketConnectorT, typename SocketContextFactoryT>
    class SocketClient : public ClientSocketT {
        /** Sequence diagramm showing how a connect to a peer is performed.
        @startuml
        !include core/socket/stream/pu/SocketClient.pu
        @enduml
        */

        SocketClient() = delete;

    protected:
        using ClientSocket = ClientSocketT;
        using SocketConnector = SocketConnectorT;
        using ClientConfig = typename SocketConnector::ClientConfig;
        using SocketContextFactory = SocketContextFactoryT;

    public:
        using Socket = typename ClientSocket::Socket;
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : clientConfig(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , _onConnect(onConnect)
            , _onConnected(onConnected)
            , _onDisconnect(onDisconnect)
            , options(options) {
        }

        virtual ~SocketClient() = default;

        using ClientSocket::connect;

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& bindAddress,
                     const std::function<void(int)>& onError) const override {
            clientConfig.setRemoteAddress(remoteAddress);
            clientConfig.setLocalAddress(bindAddress);

            SocketConnector* socketConnector =
                new SocketConnector(clientConfig, socketContextFactory, _onConnect, _onConnected, _onDisconnect, options);

            socketConnector->connect(remoteAddress, bindAddress, onError);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(int)>& onError) const override {
            connect(remoteAddress, SocketAddress(), onError);
        }

        void connect(const std::function<void(int)>& onError) const override {
            connect(clientConfig.getRemoteAddress(), clientConfig.getLocalAddress(), onError);
        }

        void onConnect(const std::function<void(SocketConnection*)>& onConnect) {
            _onConnect = onConnect;
        }

        void onConnected(const std::function<void(SocketConnection*)>& onConnected) {
            _onConnected = onConnected;
        }

        void onDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            _onDisconnect = onDisconnect;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() {
            return socketContextFactory;
        }

    protected:
        ClientConfig clientConfig;

        std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> _onConnect;
        std::function<void(SocketConnection*)> _onConnected;
        std::function<void(SocketConnection*)> _onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
