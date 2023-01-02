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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/socket/stream/SocketConnectionFactory.h"
#include "net/config/ConfigCluster.h"
#include "net/un/dgram/Socket.h"

namespace core::socket {
    class SocketContextFactory;
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketServerT, template <typename SocketT> class SocketConnectionT>
    class SocketAcceptor
        : protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver {
    private:
        using SocketServer = SocketServerT;
        using PrimarySocket = typename SocketServer::Socket;
        using SecondarySocket = net::un::dgram::Socket;

    protected:
        using SocketConnection = SocketConnectionT<PrimarySocket>;
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<SocketServer, SocketConnection>;

    public:
        using Config = typename SocketServer::Config;
        using SocketAddress = typename PrimarySocket::SocketAddress;

        /** Sequence diagramm of res.upgrade(req).
        @startuml
        !include core/socket/stream/pu/SocketAcceptor.pu!0
        @enduml
        */

        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;

        SocketAcceptor& operator=(const SocketAcceptor&) = delete;

        SocketAcceptor(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, int)>& onError,
                       const std::map<std::string, std::any>& options,
                       const std::shared_ptr<Config>& config)
            : core::eventreceiver::InitAcceptEventReceiver("SocketAcceptor")
            , core::eventreceiver::AcceptEventReceiver("SocketAcceptor")
            , socketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect)
            , onError(onError)
            , options(options)
            , config(config) {
            InitAcceptEventReceiver::publish();
        }

        ~SocketAcceptor() override {
            if (secondarySocket != nullptr) {
                delete secondarySocket;
            }
            if (primarySocket != nullptr) {
                delete primarySocket;
            }
        }

    protected:
        void initAcceptEvent() override {
            if (config->getClusterMode() == net::config::ConfigCluster::MODE::STANDALONE ||
                config->getClusterMode() == net::config::ConfigCluster::MODE::PRIMARY) {
                primarySocket = new PrimarySocket();
                if (primarySocket->open(PrimarySocket::Flags::NONBLOCK) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
#if !defined(NDEBUG)
                } else if (primarySocket->reuseAddress() < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
#endif // !defined(NDEBUG)
                } else if (primarySocket->bind(config->getLocalAddress()) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (primarySocket->listen(config->getBacklog()) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (config->getClusterMode() == net::config::ConfigCluster::MODE::PRIMARY) {
                    VLOG(0) << config->getName() << " mode: PRIMARY";
                    secondarySocket = new SecondarySocket();
                    if (secondarySocket->open(SecondarySocket::Flags::NONBLOCK) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else if (secondarySocket->bind(SecondarySocket::SocketAddress("/tmp/primary-" + config->getName())) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else {
                        onError(config->getLocalAddress(), 0);
                        enable(primarySocket->getFd());
                    }
                } else {
                    VLOG(0) << config->getName() << " mode: STANDALONE";
                    onError(config->getLocalAddress(), 0);
                    enable(primarySocket->getFd());
                }
            } else if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::MODE::PROXY) {
                secondarySocket = new SecondarySocket();
                if (secondarySocket->open(SecondarySocket::Flags::NONBLOCK) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (secondarySocket->bind(SecondarySocket::SocketAddress("/tmp/secondary-" + config->getName())) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else {
                    VLOG(0) << config->getName() << " mode: SECONDARY or PROXY";
                    onError(config->getLocalAddress(), errno);
                    enable(secondarySocket->getFd());
                }
            }
        }

    private:
        void acceptEvent() override {
            if (config->getClusterMode() == net::config::ConfigCluster::MODE::STANDALONE ||
                config->getClusterMode() == net::config::ConfigCluster::MODE::PRIMARY) {
                PrimarySocket socket;

                int acceptsPerTick = config->getAcceptsPerTick();

                do {
                    SocketAddress remoteAddress{};
                    socket = primarySocket->accept4(remoteAddress, SOCK_NONBLOCK);
                    if (socket.isValid()) {
                        if (config->getClusterMode() == net::config::ConfigCluster::MODE::STANDALONE) {
                            socketConnectionFactory.create(socket, config);
                        } else {
                            // Send descriptor to SECONDARY
                            VLOG(0) << "Sending to secondary";
                            secondarySocket->sendFd(SecondarySocket::SocketAddress("/tmp/secondary-" + config->getName()), socket.getFd());
                            SecondarySocket::SocketAddress address;
                        }
                    } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                        PLOG(ERROR) << "accept";
                    }
                } while (--acceptsPerTick > 0 && socket.isValid());
            } else if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::MODE::PROXY) {
                // Receive socketfd via SOCK_UNIX, SOCK_DGRAM
                int fd = -1;

                if (secondarySocket->recvFd(&fd) >= 0) {
                    PrimarySocket socket(fd);

                    if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY) {
                        socketConnectionFactory.create(socket, config);
                    } else { // PROXY
                        // Send to SECONDARY (TERTIARY)
                    }
                } else {
                    PLOG(ERROR) << "read_fd";
                }
            }
        }

    protected:
        void destruct() {
            delete this;
        }

    private:
        int reuseAddress() {
            int sockopt = 1;

            return primarySocket->setSockopt(SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
        }

        void unobservedEvent() override {
            destruct();
        }

        PrimarySocket* primarySocket = nullptr;
        SecondarySocket* secondarySocket = nullptr;

        SocketConnectionFactory socketConnectionFactory;

    protected:
        std::function<void(const SocketAddress&, int)> onError = nullptr;

        std::map<std::string, std::any> options;
        std::shared_ptr<Config> config;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
