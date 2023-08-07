/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/LogicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketConnectorT, typename SocketContextFactoryT>
    class SocketClient : public core::socket::LogicalSocket<typename SocketConnectorT::Config> {
        /** Sequence diagramm showing how a connect to a peer is performed.
        @startuml
        !include core/socket/stream/pu/SocketClient.pu
        @enduml
        */
    private:
        using SocketConnector = SocketConnectorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::LogicalSocket<typename SocketConnector::Config>;

    public:
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnector::SocketAddress;

        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : SocketClient("", onConnect, onConnected, onDisconnect) {
        }

        explicit SocketClient(const std::string& name)
            : SocketClient(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      VLOG(0) << "OnConnect " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      VLOG(0) << "OnConnected " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      VLOG(0) << "OnDisconnect " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  }) {
        }

        SocketClient()
            : SocketClient("") {
        }

        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     SocketContextFactory* socketContextFactory)
            : Super(name)
            , socketContextFactory(std::shared_ptr<SocketContextFactory>(socketContextFactory))
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     SocketContextFactory* socketContextFactory)
            : SocketClient("", onConnect, onConnected, onDisconnect, socketContextFactory) {
        }

        SocketClient(const std::string& name, SocketContextFactory* socketContextFactory)
            : SocketClient(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      VLOG(0) << "OnConnect " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      VLOG(0) << "OnConnected " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      VLOG(0) << "OnDisconnect " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  },
                  socketContextFactory) {
        }

        explicit SocketClient(SocketContextFactory* socketContextFactory)
            : SocketClient("", socketContextFactory) {
        }

        void connect(const std::function<void(const SocketAddress&, int)>& onError) const {
            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, Super::config);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Remote::setSocketAddress(remoteAddress);

            connect(onError);
        }

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& localAddress,
                     const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Local::setSocketAddress(localAddress);

            connect(remoteAddress, onError);
        }

        void setOnConnect(const std::function<void(SocketConnection*)>& onConnect) {
            this->onConnect = onConnect;
        }

        void setOnConnected(const std::function<void(SocketConnection*)>& onConnected) {
            this->onConnected = onConnected;
        }

        void setOnDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            this->onDisconnect = onDisconnect;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() {
            return socketContextFactory;
        }

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory;

    protected:
        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
