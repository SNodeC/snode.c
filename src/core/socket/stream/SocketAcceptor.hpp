/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/State.h"
#include "core/socket/stream/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <iomanip>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getLocalSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr localSockAddr;
        typename SocketAddress::SockLen localSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress localPeerAddress;
        if (physicalSocket.getSockName(localSockAddr, localSockAddrLen) == 0) {
            try {
                localPeerAddress = config->Local::getSocketAddress(localSockAddr, localSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (local): " << localPeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (local): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (local) not retrievable";
        }

        return localPeerAddress;
    }

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getRemoteSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr remoteSockAddr;
        typename SocketAddress::SockLen remoteSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress remotePeerAddress;
        if (physicalSocket.getPeerName(remoteSockAddr, remoteSockAddrLen) == 0) {
            try {
                remotePeerAddress = config->Remote::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (remote): " << remotePeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (remote): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (remote) not retrievable";
        }

        return remotePeerAddress;
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(
        const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::AcceptEventReceiver(config->getInstanceName() + " SocketAcceptor", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onStatus(onStatus)
        , config(config) {
        atNextTick([this]() {
            if (core::eventLoopState() == core::State::RUNNING) {
                init();
            } else {
                destruct();
            }
        });
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : core::eventreceiver::AcceptEventReceiver(socketAcceptor.config->getInstanceName() + " SocketAcceptor", 0)
        , socketContextFactory(socketAcceptor.socketContextFactory)
        , onConnect(socketAcceptor.onConnect)
        , onConnected(socketAcceptor.onConnected)
        , onDisconnect(socketAcceptor.onDisconnect)
        , onStatus(socketAcceptor.onStatus)
        , config(socketAcceptor.config) {
        atNextTick([this]() {
            if (core::eventLoopState() == core::State::RUNNING) {
                init();
            } else {
                destruct();
            }
        });
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::init() {
        if (!config->getDisabled()) {
            try {
                LOG(TRACE) << config->getInstanceName() << " Starting";

                localAddress = config->Local::getSocketAddress();

                core::socket::State state = core::socket::STATE_OK;

                if (physicalServerSocket.open(config->getSocketOptions(), PhysicalServerSocket::Flags::NONBLOCK) < 0) {
                    switch (errno) {
                        case EMFILE:
                        case ENFILE:
                        case ENOBUFS:
                        case ENOMEM:
                            PLOG(DEBUG) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            PLOG(DEBUG) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_FATAL;
                            break;
                    }
                } else if (physicalServerSocket.bind(localAddress) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            PLOG(DEBUG) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            PLOG(DEBUG) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_FATAL;
                            break;
                    }
                } else if (physicalServerSocket.listen(config->getBacklog()) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            PLOG(DEBUG) << config->getInstanceName() << " listen: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            PLOG(DEBUG) << config->getInstanceName() << " listen: '" << localAddress.toString() << "'";

                            state = core::socket::STATE_FATAL;
                            break;
                    }
                } else {
                    if (enable(physicalServerSocket.getFd())) {
                        LOG(DEBUG) << config->getInstanceName() << " enabled: '" << localAddress.toString() << "' success";
                    } else {
                        LOG(DEBUG) << config->getInstanceName() << " enabled: '" << localAddress.toString() << "' failed";

                        state = core::socket::STATE(core::socket::STATE_FATAL, ECANCELED, "SocketAcceptor not enabled");
                    }
                }

                SocketAddress currentLocalAddress = localAddress;
                if (localAddress.useNext()) {
                    onStatus(currentLocalAddress, (state | core::socket::State::NO_RETRY));

                    LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '"
                               << config->Local::getSocketAddress().toString() << "'";

                    useNextSocketAddress();
                } else {
                    onStatus(currentLocalAddress, state);
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(DEBUG) << config->getInstanceName() << " " << badSocketAddress.what();

                onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
            }
        } else {
            LOG(DEBUG) << config->getInstanceName() << " disabled";

            onStatus({}, core::socket::STATE_DISABLED);
        }

        if (isEnabled()) {
            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::acceptEvent() {
        int acceptsPerTick = config->getAcceptsPerTick();

        do {
            PhysicalServerSocket connectedPhysicalSocket(physicalServerSocket.accept4(PhysicalServerSocket::Flags::NONBLOCK),
                                                         physicalServerSocket.getBindAddress());
            if (connectedPhysicalSocket.isValid()) {
                LOG(DEBUG) << "[" << connectedPhysicalSocket.getFd() << " ]" << config->getInstanceName() << ": accept success: '"
                           << connectedPhysicalSocket.getBindAddress().toString() << "'";

                SocketConnection* socketConnection =
                    new SocketConnection(config->getInstanceName(),
                                         std::move(connectedPhysicalSocket),
                                         onDisconnect,
                                         localAddress.toString(false),
                                         getLocalSocketAddress<SocketAddress>(connectedPhysicalSocket, config),
                                         getRemoteSocketAddress<SocketAddress>(connectedPhysicalSocket, config),
                                         config->getReadTimeout(),
                                         config->getWriteTimeout(),
                                         config->getReadBlockSize(),
                                         config->getWriteBlockSize(),
                                         config->getTerminateTimeout());

                onConnect(socketConnection);
                onConnected(socketConnection);
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(WARNING) << config->getInstanceName() << " accept failed: '" << physicalServerSocket.getBindAddress().toString()
                              << "'";
            }
        } while (--acceptsPerTick > 0);
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::destruct() {
        delete this;
    }

} // namespace core::socket::stream
