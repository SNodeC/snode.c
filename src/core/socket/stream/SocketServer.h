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

#ifndef CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
#define CORE_SOCKET_STREAM_SOCKETSERVERNEW_H

#include "core/SNodeC.h"
#include "core/socket/Socket.h" // IWYU pragma: export
#include "core/socket/State.h"  // IWYU pragma: export
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <memory>
#include <random>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketAcceptorT, typename SocketContextFactoryT>
    class SocketServer : public core::socket::Socket<typename SocketAcceptorT::Config> {
    private:
        using SocketAcceptor = SocketAcceptorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::Socket<typename SocketAcceptor::Config>;

    public:
        using SocketConnection = typename SocketAcceptor::SocketConnection;
        using SocketAddress = typename SocketAcceptor::SocketAddress;

        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>())
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        SocketServer(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : SocketServer("", onConnect, onConnected, onDisconnect) {
        }

        explicit SocketServer(const std::string& name)
            : SocketServer(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      LOG(INFO) << "OnConnect - " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      LOG(INFO) << "OnConnected - " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      LOG(INFO) << "OnDisconnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  }) {
        }

        explicit SocketServer()
            : SocketServer("") {
        }

        SocketServer(const std::string& name,
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

        SocketServer(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     SocketContextFactory* socketContextFactory)
            : SocketServer("", onConnect, onConnected, onDisconnect, socketContextFactory) {
        }

        SocketServer(const std::string& name, SocketContextFactory* socketContextFactory)
            : SocketServer(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      LOG(INFO) << "OnConnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      LOG(INFO) << "OnConnected " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      LOG(INFO) << "OnDisconnect " << name;

                      LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                                       socketConnection->getLocalAddress().toString();
                      LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                                       socketConnection->getRemoteAddress().toString();
                  },
                  socketContextFactory) {
        }

        explicit SocketServer(SocketContextFactory* socketContextFactory)
            : SocketServer("", socketContextFactory) {
        }

    private:
        void realListen(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                        unsigned int tries,
                        double retryTimeoutScale) const {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketAcceptor(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    onDisconnect,
                    [server = *this, onStatus, tries, retryTimeoutScale](const SocketAddress& socketAddress,
                                                                         core::socket::State state) -> void {
                        onStatus(socketAddress, state);

                        switch (state) {
                            case core::socket::State::ERROR:
                                if (server.getConfig().getRetry() &&
                                    (server.getConfig().getRetryTries() == 0 || tries < server.getConfig().getRetryTries())) {
                                    double relativeRetryTimeout =
                                        server.getConfig().getRetryLimit() > 0
                                            ? std::min<double>(server.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                               server.getConfig().getRetryLimit())
                                            : server.getConfig().getRetryTimeout() * retryTimeoutScale;
                                    relativeRetryTimeout -= utils::Random::getInRange(-server.getConfig().getRetryJitter(),
                                                                                      server.getConfig().getRetryJitter()) *
                                                            relativeRetryTimeout / 100.;

                                    LOG(INFO) << "OnStatus: " << server.getConfig().getInstanceName();
                                    LOG(INFO) << "  retrying in " << relativeRetryTimeout << " seconds";

                                    core::timer::Timer::singleshotTimer(
                                        [server, onStatus, tries, retryTimeoutScale]() mutable -> void {
                                            server.getConfig().Local::reset();
                                            server.realListen(onStatus, tries + 1, retryTimeoutScale * server.getConfig().getRetryBase());
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
        void listen(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            realListen(onStatus, 0, 1);
        }

        void listen(const SocketAddress& localAddress,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setSocketAddress(localAddress);

            listen(onStatus);
        }

        void listen(const SocketAddress& localAddress,
                    int backlog,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setBacklog(backlog);

            listen(localAddress, onStatus);
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

#endif // CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
