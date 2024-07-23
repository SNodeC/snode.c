/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
#include "core/socket/State.h"  // IWYU pragma: export
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketConnectorT, typename SocketContextFactoryT, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
    class SocketClient : public core::socket::Socket<typename SocketConnectorT::Config> {
        /** Sequence diagram showing how a connect to a peer is performed.
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
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>(std::forward<Args>(args)...))
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : SocketClient("", onConnect, onConnected, onDisconnect, std::forward<Args>(args)...) {
        }

        SocketClient(const std::string& name, Args&&... args)
            : SocketClient(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      LOG(INFO) << "OnConnect " << name;

                      LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer: " << socketConnection->getRemoteAddress().toString();
                  },
                  [name](SocketConnection* socketConnection) -> void { // onConnected
                      LOG(INFO) << "OnConnected " << name;

                      LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      LOG(INFO) << "OnDisconnect " << name;

                      LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
                  },
                  std::forward<Args>(args)...) {
        }

        explicit SocketClient(Args&&... args)
            : SocketClient("", std::forward<Args>(args)...) {
        }

    private:
        void realConnect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                         unsigned int tries,
                         double retryTimeoutScale) const {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketConnector(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    [client = *this, onStatus](SocketConnection* socketConnection) -> void {
                        client.onDisconnect(socketConnection);

                        if (client.getConfig().getReconnect() && core::eventLoopState() == core::State::RUNNING) {
                            double relativeReconnectTimeout = client.getConfig().getReconnectTime();

                            LOG(INFO) << "Client OnDisconnect: " << client.getConfig().getInstanceName();
                            LOG(INFO) << "  "
                                      << "  reconnecting in " << relativeReconnectTimeout << " seconds";

                            core::timer::Timer::singleshotTimer(
                                [client, onStatus]() -> void {
                                    client.getConfig().Local::renew();
                                    client.getConfig().Remote::renew();

                                    client.realConnect(onStatus, 0, 1);
                                },
                                relativeReconnectTimeout);
                        }
                    },
                    [client = *this, onStatus, tries, retryTimeoutScale](const SocketAddress& socketAddress,
                                                                         core::socket::State state) -> void {
                        bool retry = (state & core::socket::State::NO_RETRY) == 0 &&
                                     (client.getConfig().getRetryTries() == 0 || tries < client.getConfig().getRetryTries());
                        state &= ~core::socket::State::NO_RETRY;

                        onStatus(socketAddress, state);

                        switch (state) {
                            case core::socket::State::OK:
                                [[fallthrough]];
                            case core::socket::State::DISABLED:
                                retry = false;
                                break;
                            case core::socket::State::ERROR:
                                retry = retry && client.getConfig().getRetry();
                                break;
                            case core::socket::State::FATAL:
                                retry = retry && client.getConfig().getRetry() && client.getConfig().getRetryOnFatal();
                                break;
                        }

                        if (retry) {
                            double relativeRetryTimeout = client.getConfig().getRetryLimit() > 0
                                                              ? std::min<double>(client.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                                                 client.getConfig().getRetryLimit())
                                                              : client.getConfig().getRetryTimeout() * retryTimeoutScale;
                            relativeRetryTimeout -=
                                utils::Random::getInRange(-client.getConfig().getRetryJitter(), client.getConfig().getRetryJitter()) *
                                relativeRetryTimeout / 100.;

                            LOG(INFO) << "Client OnStatus: " << client.getConfig().getInstanceName();
                            LOG(INFO) << "  "
                                      << " retrying in " << relativeRetryTimeout << " seconds";

                            core::timer::Timer::singleshotTimer(
                                [client, onStatus, tries, retryTimeoutScale]() mutable -> void {
                                    client.getConfig().Local::renew();
                                    client.getConfig().Remote::renew();

                                    client.realConnect(onStatus, tries + 1, retryTimeoutScale * client.getConfig().getRetryBase());
                                },
                                relativeRetryTimeout);
                        }
                    },
                    Super::config);
            }
        }

    public:
        void connect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            realConnect(onStatus, 0, 1);
        }

        void connect(const SocketAddress& remoteAddress,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Remote::setSocketAddress(remoteAddress);

            connect(onStatus);
        }

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& localAddress,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setSocketAddress(localAddress);

            connect(remoteAddress, onStatus);
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

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() const {
            return socketContextFactory;
        }

    private:
        const std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
