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

    template <typename PhysicalClientSocketT, typename ConfigT, typename SocketConnectorT, typename SocketContextFactoryT>
    class SocketClient : public core::socket::LogicalSocket<PhysicalClientSocketT, ConfigT> {
        /** Sequence diagramm showing how a connect to a peer is performed.
        @startuml
        !include core/socket/stream/pu/SocketClient.pu
        @enduml
        */
    private:
        using Super = core::socket::LogicalSocket<PhysicalClientSocketT, ConfigT>;
        using SocketConnector = SocketConnectorT;
        using SocketContextFactory = SocketContextFactoryT;

    protected:
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

    public:
        SocketClient() = delete;

        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , _onConnect(onConnect)
            , _onConnected(onConnected)
            , _onDisconnect(onDisconnect) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : SocketClient("", onConnect, onConnected, onDisconnect) {
        }

        void connect(const std::function<void(const SocketAddress&, int)>& onError) const {
            new SocketConnector(socketContextFactory, _onConnect, _onConnected, _onDisconnect, onError, Super::config);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Remote::setAddress(remoteAddress);

            connect(onError);
        }

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& localAddress,
                     const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Local::setAddress(localAddress);

            connect(remoteAddress, onError);
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

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory;

    protected:
        std::function<void(SocketConnection*)> _onConnect;
        std::function<void(SocketConnection*)> _onConnected;
        std::function<void(SocketConnection*)> _onDisconnect;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
