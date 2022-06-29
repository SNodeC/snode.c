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
        : /*protected ServerSocketT::Socket
        ,*/
          protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver
        , protected core::socket::stream::SocketConnectionEstablisher<ServerSocketT, SocketConnectionT> {
        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(const SocketAcceptor&) = delete;

    private:
        using ServerSocket = ServerSocketT;
        using Socket = typename ServerSocket::Socket;

    protected:
        using SocketConnection = SocketConnectionT<Socket>;
        using SocketConnectionEstablisher = core::socket::stream::SocketConnectionEstablisher<ServerSocketT, SocketConnectionT>;

    public:
        using Config = typename ServerSocket::Config;
        using SocketAddress = typename Socket::SocketAddress;

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
            , SocketConnectionEstablisher(socketContextFactory, onConnect, onConnected, onDisconnect)
            , options(options) {
        }

        ~SocketAcceptor() override {
            if (dgramSocket != nullptr) {
                delete dgramSocket;
            }
            if (streamSocket != nullptr) {
                delete streamSocket;
            }
        }

        void listen(const std::shared_ptr<Config>& config, const std::function<void(const SocketAddress&, int)>& onError) {
            this->config = config;
            this->onError = onError;

            InitAcceptEventReceiver::publish();
        }

    private:
        void initAcceptEvent() override {
            if (!config->isCluster() || config->getClusterMode() == net::config::ConfigCluster::PRIMARY) {
                VLOG(0) << "Cluster: BARE or PRIMARY";

                streamSocket = new Socket();
                if (streamSocket->open(SOCK_NONBLOCK) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
#if !defined(NDEBUG)
                } else if (reuseAddress()) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
#endif // !defined(NDEBUG)
                } else if (streamSocket->bind(config->getLocalAddress()) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (core::system::listen(streamSocket->getFd(), config->getBacklog()) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (config->isCluster() && config->getClusterMode() == net::config::ConfigCluster::PRIMARY) {
                    VLOG(0) << "Cluster: PRIMARY";
                    dgramSocket = new net::un::dgram::Socket();
                    if (dgramSocket->open(SOCK_NONBLOCK) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else if (dgramSocket->bind(net::un::SocketAddress("/tmp/primary")) < 0) {
                        onError(config->getLocalAddress(), errno);
                        destruct();
                    } else {
                        onError(config->getLocalAddress(), 0);
                        enable(streamSocket->getFd());
                    }
                } else {
                    onError(config->getLocalAddress(), 0);
                    enable(streamSocket->getFd());
                }
            } else if (config->getClusterMode() == net::config::ConfigCluster::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::PROXY) {
                VLOG(0) << "Cluster: SECONDARY or PROXY";
                dgramSocket = new net::un::dgram::Socket();
                if (dgramSocket->open(SOCK_NONBLOCK) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else if (dgramSocket->bind(net::un::SocketAddress("/tmp/secondary")) < 0) {
                    onError(config->getLocalAddress(), errno);
                    destruct();
                } else {
                    onError(config->getLocalAddress(), 0);
                    enable(dgramSocket->getFd());
                }
            }
        }

        void acceptEvent() override {
            if (!config->isCluster() || config->getClusterMode() == net::config::ConfigCluster::PRIMARY) {
                int fd = -1;

                int acceptsPerTick = config->getAcceptsPerTick();

                do {
                    typename SocketAddress::SockAddr remoteAddress{};
                    socklen_t remoteAddressLength = sizeof(remoteAddress);

                    fd = core::system::accept4(
                        streamSocket->getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &remoteAddressLength, SOCK_NONBLOCK);

                    if (!config->isCluster()) {
                        if (fd >= 0) {
                            typename SocketAddress::SockAddr localAddress{};
                            socklen_t addressLength = sizeof(localAddress);

                            if (core::system::getsockname(fd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                                SocketConnectionEstablisher::establishConnection(fd, localAddress, remoteAddress, config);
                            } else {
                                PLOG(ERROR) << "getsockname";
                                core::system::shutdown(fd, SHUT_RDWR);
                                core::system::close(fd);
                            }
                        }
                    } else {
                        // Send descriptor to SECONDARY
                        VLOG(0) << "Sending to secondary";
                        dgramSocket->write_fd(net::un::Socket::SocketAddress("/tmp/secondary"), (void*) "", 1, fd);
                        core::system::close(fd);
                    }
                } while (fd >= 0 && --acceptsPerTick > 0);

                if (fd < 0 && errno != EINTR && errno != EAGAIN) {
                    PLOG(ERROR) << "accept";
                }
            } else if (config->getClusterMode() == net::config::ConfigCluster::SECONDARY ||
                       config->getClusterMode() == net::config::ConfigCluster::PROXY) {
                int fd = -1;
                // Receive socketfd via SOCK_UNIX, SOCK_DGRAM

                char buf[10];
                if (dgramSocket->read_fd((void*) buf, 10, &fd) >= 0) {
                    if (config->getClusterMode() == net::config::ConfigCluster::SECONDARY) {
                        typename SocketAddress::SockAddr localAddress{};
                        socklen_t localAddressLength = sizeof(localAddress);

                        typename SocketAddress::SockAddr remoteAddress{};
                        socklen_t remoteAddressLength = sizeof(remoteAddress);

                        if (core::system::getsockname(fd, reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) == 0 &&
                            core::system::getpeername(fd, reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) == 0) {
                            SocketConnectionEstablisher::establishConnection(fd, localAddress, remoteAddress, config);
                        } else {
                            PLOG(ERROR) << "getsockname";
                            core::system::shutdown(fd, SHUT_RDWR);
                            core::system::close(fd);
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

            return core::system::setsockopt(streamSocket->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
        }

        void unobservedEvent() override {
            destruct();
        }

        std::function<void(const SocketAddress&, int)> onError = nullptr;

    protected:
        std::map<std::string, std::any> options;

    private:
        net::un::dgram::Socket* dgramSocket = nullptr;
        Socket* streamSocket = nullptr;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
