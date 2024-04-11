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
#include "core/socket/stream/SocketConnector.h"

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
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (local): " << localPeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (local): " << badSocketAddress.what();

                try {
                    localPeerAddress = config->Local::Super::getSocketAddress(localSockAddr, localSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (local): " << localPeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (local): " << badSocketAddress.what();
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
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (remote): " << remotePeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (remote): " << badSocketAddress.what();

                try {
                    remotePeerAddress = config->Remote::Super::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (remote): " << remotePeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                    LOG(TRACE) << config->getInstanceName() << std::setw(24) << " PeerAddress (remote): " << badSocketAddress.what();
                }
            }
        }

        return remotePeerAddress;
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::ConnectEventReceiver(config->getInstanceName() + " SocketConnector:", 0)
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
    SocketConnector<PhysicalSocketServer, Config, SocketConnection>::SocketConnector(const SocketConnector& socketConnector)
        : core::eventreceiver::ConnectEventReceiver(socketConnector.config->getInstanceName() + " SocketConnector:", 0)
        , socketContextFactory(socketConnector.socketContextFactory)
        , onConnect(socketConnector.onConnect)
        , onConnected(socketConnector.onConnected)
        , onDisconnect(socketConnector.onDisconnect)
        , onStatus(socketConnector.onStatus)
        , config(socketConnector.config) {
        atNextTick([this]() -> void {
            init();
        });
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::~SocketConnector() {
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::init() {
        if (!config->getDisabled() && core::eventLoopState() == core::State::RUNNING) {
            try {
                LOG(TRACE) << config->getInstanceName() << ": Starting";

                remoteAddress = config->Remote::getSocketAddress();
                SocketAddress localAddress = config->Local::getSocketAddress();

                try {
                    if (physicalClientSocket.open(config->getSocketOptions(), PhysicalClientSocket::Flags::NONBLOCK) < 0) {
                        core::socket::State state = core::socket::STATE_OK;

                        switch (errno) {
                            case EMFILE:
                            case ENFILE:
                            case ENOBUFS:
                            case ENOMEM:
                                PLOG(TRACE) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(TRACE) << config->getInstanceName() << ": open failed '" << localAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        onStatus(remoteAddress, state);
                    } else if (physicalClientSocket.bind(localAddress) < 0) {
                        core::socket::State state = core::socket::STATE_OK;

                        switch (errno) {
                            case EADDRINUSE:
                                PLOG(TRACE) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(TRACE) << config->getInstanceName() << ": bind failed '" << localAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        onStatus(remoteAddress, state);
                    } else if (physicalClientSocket.connect(remoteAddress) < 0 && !PhysicalClientSocket::connectInProgress(errno)) {
                        core::socket::State state = core::socket::STATE_OK;

                        switch (errno) {
                            case EADDRINUSE:
                            case EADDRNOTAVAIL:
                            case ECONNREFUSED:
                            case ENETUNREACH:
                            case ENOENT:
                            case EHOSTDOWN:
                                PLOG(TRACE) << config->getInstanceName() << ": connect '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        if (remoteAddress.useNext()) {
                            onStatus(remoteAddress, state | core::socket::State::NO_RETRY);

                            LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '"
                                       << config->Remote::getSocketAddress().toString() << "'";

                            useNextSocketAddress();
                        } else {
                            onStatus(remoteAddress, state);
                        }
                    } else if (PhysicalClientSocket::connectInProgress(errno)) {
                        if (enable(physicalClientSocket.getFd())) {
                            LOG(TRACE) << config->getInstanceName() << ": connect in progress '" << remoteAddress.toString() << "'";
                        } else {
                            LOG(TRACE) << config->getInstanceName() << ": Error: not monitored by SNode.C";
                        }
                    } else {
                        LOG(TRACE) << config->getInstanceName() << ": connect success '" << remoteAddress.toString() << "'";

                        onStatus(remoteAddress, core::socket::STATE_OK);

                        SocketConnection* socketConnection =
                            new SocketConnection(config->getInstanceName(),
                                                 std::move(physicalClientSocket),
                                                 onDisconnect,
                                                 remoteAddress.toString(false),
                                                 getLocalSocketAddress<SocketAddress>(physicalClientSocket, config),
                                                 getRemoteSocketAddress<SocketAddress>(physicalClientSocket, config),
                                                 config->getReadTimeout(),
                                                 config->getWriteTimeout(),
                                                 config->getReadBlockSize(),
                                                 config->getWriteBlockSize(),
                                                 config->getTerminateTimeout());

                        onConnect(socketConnection);
                        onConnected(socketConnection);
                    }
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    LOG(TRACE) << config->getInstanceName() << ": " << badSocketAddress.what();

                    onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << ": " << badSocketAddress.what();

                onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
            }
        } else {
            LOG(TRACE) << config->getInstanceName() << ": disabled";

            onStatus({}, core::socket::STATE_DISABLED);
        }

        if (isEnabled()) {
            core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectEvent() {
        int cErrno = 0;
        if (physicalClientSocket.getSockError(cErrno) == 0) { //  == 0->return valid : < 0->getsockopt failed
            const utils::PreserveErrno pe(cErrno);            // errno = cErrno

            if (errno == 0) {
                LOG(TRACE) << config->getInstanceName() << ": connect success '" << remoteAddress.toString() << "'";

                onStatus(remoteAddress, core::socket::STATE_OK);

                SocketConnection* socketConnection =
                    new SocketConnection(config->getInstanceName(),
                                         std::move(physicalClientSocket),
                                         onDisconnect,
                                         remoteAddress.toString(false),
                                         getLocalSocketAddress<SocketAddress>(physicalClientSocket, config),
                                         getRemoteSocketAddress<SocketAddress>(physicalClientSocket, config),
                                         config->getReadTimeout(),
                                         config->getWriteTimeout(),
                                         config->getReadBlockSize(),
                                         config->getWriteBlockSize(),
                                         config->getTerminateTimeout());

                onConnect(socketConnection);
                onConnected(socketConnection);

                disable();
            } else if (PhysicalClientSocket::connectInProgress(errno)) {
                LOG(TRACE) << config->getInstanceName() << ": connect still in progress '" << remoteAddress.toString() << "'";
            } else if (remoteAddress.useNext()) {
                core::socket::State state = core::socket::STATE_OK;

                switch (errno) {
                    case EADDRINUSE:
                    case EADDRNOTAVAIL:
                    case ECONNREFUSED:
                    case ENETUNREACH:
                    case ENOENT:
                    case EHOSTDOWN:
                        PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                        state = core::socket::STATE_ERROR;
                        break;
                    default:
                        PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                        state = core::socket::STATE_FATAL;
                        break;
                }

                onStatus(remoteAddress, (state | core::socket::State::NO_RETRY));

                LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '" << config->Remote::getSocketAddress().toString()
                           << "'";

                useNextSocketAddress();

                disable();
            } else {
                core::socket::State state = core::socket::STATE_OK;

                switch (errno) {
                    case EADDRINUSE:
                    case EADDRNOTAVAIL:
                    case ECONNREFUSED:
                    case ENETUNREACH:
                    case ENOENT:
                    case EHOSTDOWN:
                        PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                        state = core::socket::STATE_ERROR;
                        break;
                    default:
                        PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                        state = core::socket::STATE_FATAL;
                        break;
                }

                onStatus(remoteAddress, state);

                disable();
            }
        } else {
            PLOG(TRACE) << config->getInstanceName() << ": getsockopt syscall error '" << remoteAddress.toString() << "'";

            onStatus(remoteAddress, core::socket::STATE_FATAL);
            disable();
        }
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectTimeout() {
        LOG(TRACE) << config->getInstanceName() << ": connect timeout " << remoteAddress.toString();

        if (remoteAddress.useNext()) {
            LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '" << config->Remote::getSocketAddress().toString()
                       << "'";

            useNextSocketAddress();
        } else {
            LOG(TRACE) << config->getInstanceName() << ": connect timeout '" << remoteAddress.toString() << "'";

            onStatus(remoteAddress, core::socket::STATE_ERROR);
        }

        disable();
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::destruct() {
        delete this;
    }

} // namespace core::socket::stream
