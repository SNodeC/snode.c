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

#include "log/Logger.h"

#include <any> // IWYU pragma: export
#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <map>        // IWYU pragma: export
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketClientT, template <typename ClientSocket> class SocketConnectorT, typename SocketContextFactoryT>
    class SocketClient : public SocketClientT {
        /** Sequence diagramm showing how a connect to a peer is performed.
        @startuml
        !include core/socket/stream/pu/SocketClient.pu
        @enduml
        */
    private:
        using Super = SocketClientT;
        using SocketConnector = SocketConnectorT<Super>;
        using SocketContextFactory = SocketContextFactoryT;

    public:
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        SocketClient() = delete;

        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , _onConnect(onConnect)
            , _onConnected(onConnected)
            , _onDisconnect(onDisconnect)
            , _options(options) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : SocketClient("", onConnect, onConnected, onDisconnect, options) {
        }

        using Super::connect;

        void connect(const std::function<void(const SocketAddress&, int)>& onError) const override {
            if (Super::config->isRemoteInitialized()) {
                new SocketConnector(socketContextFactory, _onConnect, _onConnected, _onDisconnect, onError, _options, Super::config);
            } else {
                LOG(ERROR) << "Parameterless connect with anonymous client instance";
            }
        }

        std::function<void(SocketConnection*)> onConnect(std::function<void(SocketConnection*)>&& onConnect) {
            std::swap(onConnect, _onConnect);
            return onConnect;
        }

        std::function<void(SocketConnection*)> onConnected(std::function<void(SocketConnection*)>&& onConnected) {
            std::swap(onConnected, _onConnected);
            return onConnected;
        }

        std::function<void(SocketConnection*)> onDisconnect(std::function<void(SocketConnection*)>&& onDisconnect) {
            std::swap(onDisconnect, _onDisconnect);
            return onDisconnect;
        }

        std::function<void(SocketConnection*)> onConnect(std::function<void(SocketConnection*)>& onConnect) {
            std::swap(onConnect, _onConnect);
            return onConnect;
        }

        std::function<void(SocketConnection*)> onConnected(std::function<void(SocketConnection*)>& onConnected) {
            std::swap(onConnected, _onConnected);
            return onConnected;
        }

        std::function<void(SocketConnection*)> onDisconnect(std::function<void(SocketConnection*)>& onDisconnect) {
            std::swap(onDisconnect, _onDisconnect);
            return onDisconnect;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() {
            return socketContextFactory;
        }

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> _onConnect;
        std::function<void(SocketConnection*)> _onConnected;
        std::function<void(SocketConnection*)> _onDisconnect;

    protected:
        std::map<std::string, std::any> _options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
