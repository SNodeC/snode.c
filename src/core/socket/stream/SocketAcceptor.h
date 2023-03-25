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

namespace core::socket::stream {
    class SocketContextFactory;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketServerT, template <typename PhysicalServerSocketT> class SocketConnectionT>
    class SocketAcceptor
        : protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver {
    private:
        using SocketServer = SocketServerT;
        using PrimaryPhysicalSocket = typename SocketServer::PhysicalSocket;
        using SecondarySocket = net::un::dgram::Socket;

    protected:
        using SocketConnection = SocketConnectionT<PrimaryPhysicalSocket>;
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<SocketServer, SocketConnection>;

    public:
        using Config = typename SocketServer::Config;
        using SocketAddress = typename PrimaryPhysicalSocket::SocketAddress;

        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor(SocketAcceptor&&) = delete;

        SocketAcceptor& operator=(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(SocketAcceptor&&) = delete;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, int)>& onError,
                       const std::shared_ptr<Config>& config)
            : core::eventreceiver::InitAcceptEventReceiver("SocketAcceptor")
            , core::eventreceiver::AcceptEventReceiver("SocketAcceptor")
            , socketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect)
            , onError(onError)
            , config(config) {
            InitAcceptEventReceiver::span();
        }

        ~SocketAcceptor() override {
            if (secondaryPhysicalSocket != nullptr) {
                delete secondaryPhysicalSocket;
            }
            if (primaryPhysicalSocket != nullptr) {
                delete primaryPhysicalSocket;
            }
        }

    protected:
        void initAcceptEvent() override {
            if (!config->getDisabled()) {
                if (config->getClusterMode() == Config::ConfigCluster::MODE::STANDALONE ||
                    config->getClusterMode() == Config::ConfigCluster::MODE::PRIMARY) {
                    primaryPhysicalSocket = new PrimaryPhysicalSocket();
                    if (primaryPhysicalSocket->open(config->getSocketOptions(), PrimaryPhysicalSocket::Flags::NONBLOCK) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else if (primaryPhysicalSocket->bind(config->Local::getAddress()) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else if (primaryPhysicalSocket->listen(config->getBacklog()) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else if (config->getClusterMode() == Config::ConfigCluster::MODE::PRIMARY) {
                        VLOG(0) << (config->getInstanceName().empty() ? "Unnamed instance" : config->getInstanceName()) << " mode: PRIMARY";
                        secondaryPhysicalSocket = new SecondarySocket();
                        if (secondaryPhysicalSocket->open(std::map<int, const core::socket::PhysicalSocketOption>(),
                                                          SecondarySocket::Flags::NONBLOCK) < 0) {
                            onError(config->Local::getAddress(), errno);
                            destruct();
                        } else if (secondaryPhysicalSocket->bind(
                                       SecondarySocket::SocketAddress("/tmp/primary-" + config->getInstanceName())) < 0) {
                            onError(config->Local::getAddress(), errno);
                            destruct();
                        } else {
                            onError(config->Local::getAddress(), 0);
                            enable(primaryPhysicalSocket->getFd());
                        }
                    } else {
                        VLOG(0) << (config->getInstanceName().empty() ? "Unnamed instance" : config->getInstanceName())
                                << " mode: STANDALONE";
                        onError(config->Local::getAddress(), 0);
                        enable(primaryPhysicalSocket->getFd());
                    }
                } else if (config->getClusterMode() == Config::ConfigCluster::MODE::SECONDARY ||
                           config->getClusterMode() == Config::ConfigCluster::MODE::PROXY) {
                    secondaryPhysicalSocket = new SecondarySocket();
                    if (secondaryPhysicalSocket->open(std::map<int, const core::socket::PhysicalSocketOption>(),
                                                      SecondarySocket::Flags::NONBLOCK) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else if (secondaryPhysicalSocket->bind(
                                   SecondarySocket::SocketAddress("/tmp/secondary-" + config->getInstanceName())) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else {
                        VLOG(0) << (config->getInstanceName().empty() ? "Unnamed instance" : config->getInstanceName())
                                << " mode: SECONDARY or PROXY";
                        onError(config->Local::getAddress(), errno);
                        enable(secondaryPhysicalSocket->getFd());
                    }
                }
            } else {
                destruct();
            }
        }

    private:
        void acceptEvent() override {
            if (config->getClusterMode() == Config::ConfigCluster::MODE::STANDALONE ||
                config->getClusterMode() == Config::ConfigCluster::MODE::PRIMARY) {
                PrimaryPhysicalSocket physicalSocket;

                int acceptsPerTick = config->getAcceptsPerTick();

                do {
                    SocketAddress remoteAddress{};
                    physicalSocket = primaryPhysicalSocket->accept4(remoteAddress, SOCK_NONBLOCK);
                    if (physicalSocket.isValid()) {
                        if (config->getClusterMode() == Config::ConfigCluster::MODE::STANDALONE) {
                            socketConnectionFactory.create(physicalSocket, config);
                        } else {
                            // Send descriptor to SECONDARY
                            VLOG(0) << "Sending to secondary";
                            secondaryPhysicalSocket->sendFd(SecondarySocket::SocketAddress("/tmp/secondary-" + config->getInstanceName()),
                                                            physicalSocket.getFd());
                        }
                    } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                        PLOG(ERROR) << "accept";
                    }
                } while (--acceptsPerTick > 0 && physicalSocket.isValid());
            } else if (config->getClusterMode() == Config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == Config::ConfigCluster::MODE::PROXY) {
                // Receive socketfd via SOCK_UNIX, SOCK_DGRAM
                int fd = -1;

                if (secondaryPhysicalSocket->recvFd(&fd) >= 0) {
                    PrimaryPhysicalSocket socket(fd);

                    if (config->getClusterMode() == Config::ConfigCluster::MODE::SECONDARY) {
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
        void destruct() override {
            delete this;
        }

    private:
        void unobservedEvent() override {
            destruct();
        }

        PrimaryPhysicalSocket* primaryPhysicalSocket = nullptr;
        SecondarySocket* secondaryPhysicalSocket = nullptr;

        SocketConnectionFactory socketConnectionFactory;

    protected:
        std::function<void(const SocketAddress&, int)> onError = nullptr;

        std::shared_ptr<Config> config;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
