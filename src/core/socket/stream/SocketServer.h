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
#include "core/socket/LogicalSocket.h" // IWYU pragma: export
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstddef>
#include <functional> // IWYU pragma: export
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketAcceptorT, typename SocketContextFactoryT>
    class SocketServer : public core::socket::LogicalSocket<typename SocketAcceptorT::Config> {
    private:
        using SocketAcceptor = SocketAcceptorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::LogicalSocket<typename SocketAcceptor::Config>;

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
                      VLOG(0) << "OnConnect - " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  },
                  [name]([[maybe_unused]] SocketConnection* socketConnection) -> void { // onConnected
                      VLOG(0) << "OnConnected - " << name;
                  },
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      VLOG(0) << "OnDisconnect " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
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

        explicit SocketServer(SocketContextFactory* socketContextFactory)
            : SocketServer("", socketContextFactory) {
        }

    private:
        void realListen(const std::function<void(const SocketAddress&, int)>& onError, unsigned int tries, double retryTimeoutScale) const {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketAcceptor(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    onDisconnect,
                    [server = *this, onError, tries, retryTimeoutScale](const SocketAddress& socketAddress, int errnum) -> void {
                        onError(socketAddress, errnum);

                        if (server.getConfig().getRetry() &&
                            (server.getConfig().getRetryTries() == 0 || tries < server.getConfig().getRetryTries())) {
                            if (errnum != 0 && server.getConfig().getRetry()) {
                                LOG(INFO) << "Retrying in "
                                          << (server.getConfig().getRetryLimit() > 0
                                                  ? std::min<double>(server.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                                     server.getConfig().getRetryLimit())
                                                  : server.getConfig().getRetryTimeout() * retryTimeoutScale)
                                          << " seconds";

                                core::timer::Timer::singleshotTimer(
                                    [server, onError, tries, retryTimeoutScale]() mutable -> void {
                                        server.realListen(onError, tries + 1, retryTimeoutScale * server.getConfig().getRetryBase());
                                    },
                                    server.getConfig().getRetryLimit() > 0
                                        ? std::min<double>(server.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                           server.getConfig().getRetryLimit())
                                        : server.getConfig().getRetryTimeout() * retryTimeoutScale);
                            }
                        }
                    },
                    Super::config);
            }
        }

    public:
        void listen(const std::function<void(const SocketAddress&, int)>& onError) const {
            realListen(onError, 0, 1);
        }

        void listen(const SocketAddress& localAddress, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Local::setSocketAddress(localAddress);

            listen(onError);
        }

        void listen(const SocketAddress& localAddress, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::config->Local::setBacklog(backlog);

            listen(localAddress, onError);
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

#endif // CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
