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

#include "core/SNodeC.h"
#include "core/socket/Socket.h" // IWYU pragma: export
#include "core/socket/State.h"  // IWYU pragme: export
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketConnectorT, typename SocketContextFactoryT>
    class SocketClient : public core::socket::Socket<typename SocketConnectorT::Config> {
        /** Sequence diagramm showing how a connect to a peer is performed.
        @startuml
        !include core/socket/stream/pu/SocketClient.pu
        @enduml
        */
    private:
        using SocketConnector = SocketConnectorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::Socket<typename SocketConnector::Config>;

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
                      LOG(INFO) << "OnConnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      LOG(INFO) << "OnConnected " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      LOG(INFO) << "OnDisconnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
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
                      LOG(INFO) << "OnConnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      LOG(INFO) << "OnConnected " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      LOG(INFO) << "OnDisconnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  socketContextFactory) {
        }

        explicit SocketClient(SocketContextFactory* socketContextFactory)
            : SocketClient("", socketContextFactory) {
        }

    private:
        void realConnect(const std::function<void(const SocketAddress&, int)>& onError, unsigned int tries, double retryTimeoutScale) {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketConnector(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    [client = *this, onError](SocketConnection* socketConnection) -> void {
                        client.onDisconnect(socketConnection);

                        if (client.getConfig().getReconnect()) {
                            double relativeReconnectTimeout = client.getConfig().getReconnectTime();

                            LOG(INFO) << "Reconnecting in " << relativeReconnectTimeout << " seconds";

                            core::timer::Timer::singleshotTimer(
                                [client, onError]() mutable -> void {
                                    client.realConnect(onError, 0, 1);
                                },
                                relativeReconnectTimeout);
                        }
                    },
                    [client = *this, onError, tries, retryTimeoutScale](const SocketAddress& socketAddress,
                                                                        core::socket::State state) -> void {
                        onError(socketAddress, state);

                        switch (state) {
                            case core::socket::State::ERROR:
                                if (client.getConfig().getRetry() &&
                                    (client.getConfig().getRetryTries() == 0 || tries < client.getConfig().getRetryTries())) {
                                    double relativeRetryTimeout =
                                        client.getConfig().getRetryLimit() > 0
                                            ? std::min<double>(client.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                               client.getConfig().getRetryLimit())
                                            : client.getConfig().getRetryTimeout() * retryTimeoutScale;
                                    relativeRetryTimeout -= utils::Random::getInRange(-client.getConfig().getRetryJitter(),
                                                                                      client.getConfig().getRetryJitter()) *
                                                            relativeRetryTimeout / 100.;

                                    LOG(INFO) << "Retrying in " << relativeRetryTimeout << " seconds";

                                    core::timer::Timer::singleshotTimer(
                                        [client, onError, tries, retryTimeoutScale]() mutable -> void {
                                            client.realConnect(onError, tries + 1, retryTimeoutScale * client.getConfig().getRetryBase());
                                        },
                                        relativeRetryTimeout);
                                }
                                break;
                            case core::socket::State::OK:
                            case core::socket::State::DISABLED:
                            case core::socket::State::FATAL:
                                break;
                        }
                    },
                    Super::config);
            }
        }

    public:
        void connect(const std::function<void(const SocketAddress&, core::socket::State)>& onError) {
            realConnect(onError, 0, 1);
        }

        void connect(const SocketAddress& remoteAddress, const std::function<void(const SocketAddress&, core::socket::State)>& onError) {
            Super::config->Remote::setSocketAddress(remoteAddress);

            connect(onError);
        }

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& localAddress,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onError) {
            Super::config->Local::setSocketAddress(localAddress);

            connect(remoteAddress, onError);
        }

        std::function<void(SocketConnection*)> setOnConnect(const std::function<void(SocketConnection*)>& onConnect) {
            std::function<void(SocketConnection*)> oldOnConnect = this->onConnect;

            this->onConnect = onConnect;

            return oldOnConnect;
        }

        std::function<void(SocketConnection*)> setOnConnected(const std::function<void(SocketConnection*)>& onConnected) {
            std::function<void(SocketConnection*)> oldOnConnected = this->onConnected;

            this->onConnected = onConnected;

            return oldOnConnected;
        }

        std::function<void(SocketConnection*)> setOnDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            std::function<void(SocketConnection*)> oldOnDisconnect = this->onDisconnect;

            this->onDisconnect = onDisconnect;

            return oldOnDisconnect;
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
