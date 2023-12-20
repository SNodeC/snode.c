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

#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: export
#include "core/socket/stream/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::InitConnectEventReceiver("SocketConnector")
        , core::eventreceiver::ConnectEventReceiver("SocketConnector", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onStatus(onStatus)
        , config(config) {
        InitConnectEventReceiver::span();
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::~SocketConnector() {
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::initConnectEvent() {
        try {
            SocketAddress localAddress = config->Local::getSocketAddress();

            try {
                remoteAddress = config->Remote::getSocketAddress();

                if (!config->getDisabled()) {
                    LOG(TRACE) << config->getInstanceName() << ": starting";

                    core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());

                    core::socket::State state = core::socket::STATE_OK;

                    if (physicalClientSocket.open(config->getSocketOptions(), PhysicalClientSocket::Flags::NONBLOCK) < 0) {
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
                    } else if (physicalClientSocket.bind(localAddress) < 0) {
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
                    } else if (physicalClientSocket.connect(remoteAddress) < 0 && !PhysicalClientSocket::connectInProgress(errno)) {
                        switch (errno) {
                            case EADDRINUSE:
                            case EADDRNOTAVAIL:
                            case ECONNREFUSED:
                            case ENETUNREACH:
                                PLOG(TRACE) << config->getInstanceName() << ": connect '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        if (remoteAddress.useNext()) {
                            LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '"
                                       << config->Remote::getSocketAddress().toString() << "'";

                            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onStatus, config);
                        } else {
                            onStatus(remoteAddress, state);
                        }
                    } else if (PhysicalClientSocket::connectInProgress(errno)) {
                        LOG(TRACE) << config->getInstanceName() << ": connect in progress '" << remoteAddress.toString() << "'";

                        enable(physicalClientSocket.getFd());
                    } else {
                        LOG(TRACE) << config->getInstanceName() << ": connect success '" << remoteAddress.toString() << "'";

                        onStatus(remoteAddress, state);

                        SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(std::move(physicalClientSocket), config);
                    }

                } else {
                    LOG(TRACE) << config->getInstanceName() << ": disabled";

                    onStatus(remoteAddress, core::socket::STATE_DISABLED);
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(TRACE) << config->getInstanceName() << ": " << badSocketAddress.what();

                onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
            }
        } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
            LOG(TRACE) << config->getInstanceName() << ": " << badSocketAddress.what();

            onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
        }

        if (!isEnabled()) {
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

                SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(std::move(physicalClientSocket), config);

                disable();
            } else if (PhysicalClientSocket::connectInProgress(errno)) {
                LOG(TRACE) << config->getInstanceName() << ": connect still in progress '" << remoteAddress.toString() << "'";
            } else if (remoteAddress.useNext()) {
                PLOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";
                LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '" << config->Remote::getSocketAddress().toString()
                           << "'";

                new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onStatus, config);

                disable();
            } else {
                core::socket::State state = core::socket::STATE_OK;

                switch (errno) {
                    case EADDRINUSE:
                    case EADDRNOTAVAIL:
                    case ECONNREFUSED:
                    case ENETUNREACH:
                        PLOG(TRACE) << config->getInstanceName() << ": con   nect failed '" << remoteAddress.toString() << "'";

                        state = core::socket::STATE_ERROR;
                        break;
                    default:
                        PLOG(TRACE) << config->getInstanceName() << ": con   nect failed '" << remoteAddress.toString() << "'";

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

            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onStatus, config);
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
