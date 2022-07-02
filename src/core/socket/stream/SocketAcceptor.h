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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/eventreceiver/InitAcceptEventReceiver.h"
#include "core/socket/stream/SocketConnectionEstablisher.h"
#include "net/config/ConfigCluster.h"
#include "net/un/dgram/Socket.h"

namespace core::socket {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"
#include "core/system/unistd.h"
#include "log/Logger.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename ServerSocketT, template <typename SocketT> class SocketConnectionT>
    class SocketAcceptor
        : protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver {
        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(const SocketAcceptor&) = delete;

    private:
        using ServerSocket = ServerSocketT;
        using PrimarySocket = typename ServerSocket::Socket;

    protected:
        using SocketConnection = SocketConnectionT<PrimarySocket>;
        using SocketConnectionEstablisher = core::socket::stream::SocketConnectionEstablisher<ServerSocketT, SocketConnectionT>;

    public:
        using Config = typename ServerSocket::Config;
        using SocketAddress = typename PrimarySocket::SocketAddress;

        /** Sequence diagramm of res.upgrade(req).
        @startuml
        !include core/socket/stream/pu/SocketAcceptor.pu!0
        @enduml
        */
        SocketAcceptor(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : core::eventreceiver::InitAcceptEventReceiver("SocketAcceptor")
            , core::eventreceiver::AcceptEventReceiver("SocketAcceptor")
            , socketConnectionEstablisher(socketContextFactory, onConnect, onConnected, onDisconnect)
            , options(options) {
        }

        ~SocketAcceptor() override {
            if (secondarySocket != nullptr) {
                delete secondarySocket;
            }
            if (primarySocket != nullptr) {
                delete primarySocket;
            }
        }

        void listen(const std::shared_ptr<Config>& config, const std::function<void(const SocketAddress&, int)>& onError) {
            this->config = config;
            this->onError = onError;

            InitAcceptEventReceiver::publish();
        }

    private:
        void initAcceptEvent() override {
            if (config->getClusterMode() == net::config::ConfigCluster::MODE::NONE ||
                config->getClusterMode() == net::config::ConfigCluster::MODE::PRIMARY) {
                VLOG(0) << "Mode: STANDALONE or PRIMARY";

                primarySocket = new PrimarySocket();
                if (primarySocket->open(PrimarySocket::SOCK::NONBLOCK) < 0) {
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
                    VLOG(0) << "    Cluster: PRIMARY";
                    secondarySocket = new net::un::dgram::Socket();
                    if (secondarySocket->open(net::un::dgram::Socket::SOCK::NONBLOCK) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else if (secondarySocket->bind(net::un::SocketAddress("/tmp/primary")) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else {
                        onError(config->getLocalAddress(), 0);
                        enable(primarySocket->getFd());
                    }
                } else {
                    VLOG(0) << "    Cluster: NONE";
                    onError(config->getLocalAddress(), 0);
                    enable(primarySocket->getFd());
                }
            } else if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::MODE::PROXY) {
                VLOG(0) << "    Mode: SECONDARY or PROXY";
                secondarySocket = new net::un::dgram::Socket();
                if (secondarySocket->open(net::un::dgram::Socket::SOCK::NONBLOCK) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (secondarySocket->bind(net::un::SocketAddress("/tmp/secondary")) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else {
                    onError(config->getLocalAddress(), 0);
                    enable(secondarySocket->getFd());
                }
            }
        }

        void acceptEvent() override {
            if (config->getClusterMode() == net::config::ConfigCluster::MODE::NONE ||
                config->getClusterMode() == net::config::ConfigCluster::MODE::PRIMARY) {
                net::Socket<SocketAddress> socket;

                int acceptsPerTick = config->getAcceptsPerTick();

                do {
                    SocketAddress remoteAddress{};
                    socket = primarySocket->accept4(remoteAddress);
                    if (socket.isValid()) {
                        if (config->getClusterMode() == net::config::ConfigCluster::MODE::NONE) {
                            SocketAddress localAddress{};
                            if (socket.getSockname(localAddress) == 0) {
                                socketConnectionEstablisher.establishConnection(socket, localAddress, remoteAddress, config);
                            } else {
                                PLOG(ERROR) << "getsockname";
                            }
                        } else {
                            // Send descriptor to SECONDARY
                            VLOG(0) << "Sending to secondary";
                            char msg = 0;
                            secondarySocket->write_fd(net::un::Socket::SocketAddress("/tmp/secondary"), &msg, 1, socket.getFd());
                        }
                    } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                        PLOG(ERROR) << "accept";
                    }
                } while (--acceptsPerTick > 0 && socket.isValid());
            } else if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::MODE::PROXY) {
                // Receive socketfd via SOCK_UNIX, SOCK_DGRAM
                int fd = -1;
                char msg;

                if (secondarySocket->read_fd(&msg, 1, &fd) >= 0) {
                    net::Socket<SocketAddress> socket(fd);

                    if (config->getClusterMode() == net::config::ConfigCluster::MODE::SECONDARY) {
                        SocketAddress localAddress{};
                        SocketAddress remoteAddress{};
                        if (socket.getSockname(localAddress) == 0 && socket.getPeername(remoteAddress) == 0) {
                            socketConnectionEstablisher.establishConnection(socket, localAddress, remoteAddress, config);
                        } else {
                            PLOG(ERROR) << "getsockname";
                        }
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

        std::shared_ptr<Config> config = nullptr;

    private:
        int reuseAddress() {
            int sockopt = 1;

            return core::system::setsockopt(primarySocket->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
        }

        void unobservedEvent() override {
            destruct();
        }

    protected:
        std::function<void(const SocketAddress&, int)> onError = nullptr;

        SocketConnectionEstablisher socketConnectionEstablisher;

        net::un::dgram::Socket* secondarySocket = nullptr;
        PrimarySocket* primarySocket = nullptr;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
