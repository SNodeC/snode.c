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

#include "core/socket/stream/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    core::socket::stream::SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::SocketAcceptor(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
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

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::initAcceptEvent() {
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
                    if (secondaryPhysicalSocket->open(SecondarySocket::Flags::NONBLOCK) < 0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else if (secondaryPhysicalSocket->bind(SecondarySocket::SocketAddress("/tmp/primary-" + config->getInstanceName())) <
                               0) {
                        onError(config->Local::getAddress(), errno);
                        destruct();
                    } else {
                        onError(config->Local::getAddress(), 0);
                        enable(primaryPhysicalSocket->getFd());
                    }
                } else {
                    VLOG(0) << (config->getInstanceName().empty() ? "Unnamed instance" : config->getInstanceName()) << " mode: STANDALONE";
                    onError(config->Local::getAddress(), 0);
                    enable(primaryPhysicalSocket->getFd());
                }
            } else if (config->getClusterMode() == Config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == Config::ConfigCluster::MODE::PROXY) {
                secondaryPhysicalSocket = new SecondarySocket();
                if (secondaryPhysicalSocket->open(SecondarySocket::Flags::NONBLOCK) < 0) {
                    onError(config->Local::getAddress(), errno);
                    destruct();
                } else if (secondaryPhysicalSocket->bind(SecondarySocket::SocketAddress("/tmp/secondary-" + config->getInstanceName())) <
                           0) {
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

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::acceptEvent() {
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

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::destruct() {
        delete this;
    }

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

} // namespace core::socket::stream
