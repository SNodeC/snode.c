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

#ifndef CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
#define CORE_SOCKET_STREAM_SOCKETSERVERNEW_H

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

    template <typename ServerSocketT, typename SocketAcceptorT, typename SocketContextFactoryT>
    class SocketServer : public ServerSocketT {
        SocketServer() = delete;

    private:
        using Super = ServerSocketT;
        using SocketAcceptor = SocketAcceptorT;
        using SocketContextFactory = SocketContextFactoryT;

    public:
        using SocketConnection = typename SocketAcceptor::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename SocketAcceptor::Config;

    public:
        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , _onConnect(onConnect)
            , _onConnected(onConnected)
            , _onDisconnect(onDisconnect)
            , options(options) {
        }

        SocketServer(const SocketServer&) = default;

        ~SocketServer() override = default;

        using Super::listen;

        void listen(const std::function<void(const SocketAddress&, int)>& onError) const override {
            if (Super::config->isLocalInitialized()) {
                SocketAcceptor* socketAcceptor = new SocketAcceptor(socketContextFactory, _onConnect, _onConnected, _onDisconnect, options);

                socketAcceptor->listen(Super::config, onError);
            } else {
                LOG(ERROR) << "Parameterless listen on anonymous server instance";
            }
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

        Config& getConfig() {
            return *Super::config;
        }

    protected:
        std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> _onConnect;
        std::function<void(SocketConnection*)> _onConnected;
        std::function<void(SocketConnection*)> _onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
