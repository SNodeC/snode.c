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
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (local): " << localPeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (local): " << badSocketAddress.what();

                try {
                    localPeerAddress = config->Local::Super::getSocketAddress(localSockAddr, localSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (local): " << localPeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (local): " << badSocketAddress.what();
                }
            }
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
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (remote): " << remotePeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (remote): " << badSocketAddress.what();

                try {
                    remotePeerAddress = config->Remote::Super::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (remote): " << remotePeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << "  PeerAddress (remote): " << badSocketAddress.what();
                }
            }
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
        : core::eventreceiver::AcceptEventReceiver(config->getInstanceName() + " SocketAcceptor:", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onStatus(onStatus)
        , config(config) {
        atNextTick([this]() -> void {
            init();
        });
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : core::eventreceiver::AcceptEventReceiver(socketAcceptor.config->getInstanceName() + " SocketAcceptor:", 0)
        , socketContextFactory(socketAcceptor.socketContextFactory)
        , onConnect(socketAcceptor.onConnect)
        , onConnected(socketAcceptor.onConnected)
        , onDisconnect(socketAcceptor.onDisconnect)
        , onStatus(socketAcceptor.onStatus)
        , config(socketAcceptor.config) {
        atNextTick([this]() -> void {
            init();
        });
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::init() {
        if (!config->getDisabled() && core::eventLoopState() == core::State::RUNNING) {
            try {
                LOG(TRACE) << config->getInstanceName() << ": Starting";

                localAddress = config->Local::getSocketAddress();

                core::socket::State state = core::socket::STATE_OK;

                if (physicalServerSocket.open(config->getSocketOptions(), PhysicalServerSocket::Flags::NONBLOCK) < 0) {
                    switch (errno) {
                        case EMFILE:
                        case ENFILE:
                        case ENOBUFS:
                        case ENOMEM:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalServerSocket.bind(localAddress) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalServerSocket.listen(config->getBacklog()) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << " listen: '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << " listen: '" << localAddress.toString() << "'";
                            break;
                    }
                } else {
                    if (enable(physicalServerSocket.getFd())) {
                        LOG(TRACE) << config->getInstanceName() << " enabled: '" << localAddress.toString() << "' success";
                    } else {
                        state = core::socket::STATE(core::socket::STATE_FATAL, ECANCELED, "SocketAcceptor not enabled");
                        LOG(TRACE) << config->getInstanceName() << " enabled: '" << localAddress.toString() << "' failed";
                    }
                }

                if (localAddress.useNext()) {
                    LOG(TRACE) << config->getInstanceName() << ": Using next SocketAddress '"
                               << config->Local::getSocketAddress().toString() << "'";

                    onStatus(localAddress, (state | core::socket::State::NO_RETRY));

                    useNextSocketAddress();
                } else {
                    onStatus(localAddress, state);
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << ": " << badSocketAddress.what();

                onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
            }
        } else {
            LOG(TRACE) << config->getInstanceName() << ": Disabled";

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
                LOG(TRACE) << config->getInstanceName() << " accept: Success '" << connectedPhysicalSocket.getBindAddress().toString()
                           << "'";

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
                PLOG(TRACE) << config->getInstanceName() << " accept: Failed '" << physicalServerSocket.getBindAddress().toString() << "'";
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
