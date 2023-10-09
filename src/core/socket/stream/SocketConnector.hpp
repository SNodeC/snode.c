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

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onError,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::InitConnectEventReceiver("SocketConnector")
        , core::eventreceiver::ConnectEventReceiver("SocketConnector", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onError(onError)
        , config(config) {
        InitConnectEventReceiver::span();
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::~SocketConnector() {
        if (physicalSocket != nullptr) {
            delete physicalSocket;
        }
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::initConnectEvent() {
        if (!config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << ": enabled";

            core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());

            SocketAddress localAddress;

            try {
                localAddress = config->Local::getSocketAddress();

                try {
                    State state = State::OK;

                    remoteAddress = config->Remote::getSocketAddress();
                    physicalSocket = new PhysicalSocket();

                    if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                        switch (errno) {
                            case EMFILE:
                            case ENFILE:
                            case ENOBUFS:
                            case ENOMEM:
                                PLOG(WARNING) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";

                                state = State::ERROR;
                                break;
                            default:
                                PLOG(ERROR) << config->getInstanceName() << ": open failed '" << localAddress.toString() << "'";

                                state = State::FATAL;
                                break;
                        }
                    } else if (physicalSocket->bind(localAddress) < 0) {
                        switch (errno) {
                            case EADDRINUSE:
                                PLOG(WARNING) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";

                                state = State::ERROR;
                                break;
                            default:
                                PLOG(ERROR) << config->getInstanceName() << ": bind failed '" << localAddress.toString() << "'";

                                state = State::FATAL;
                                break;
                        }
                    } else if (physicalSocket->connect(remoteAddress) < 0 && !physicalSocket->connectInProgress(errno)) {
                        switch (errno) {
                            case EADDRINUSE:
                            case EADDRNOTAVAIL:
                            case ECONNREFUSED:
                            case ENETUNREACH:
                                PLOG(WARNING) << config->getInstanceName() << ": connect '" << localAddress.toString() << "'";

                                state = State::ERROR;
                                break;
                            default:
                                PLOG(ERROR) << config->getInstanceName() << ": connect failed '" << localAddress.toString() << "'";

                                state = State::FATAL;
                                break;
                        }
                    } else if (physicalSocket->connectInProgress(errno)) {
                        LOG(TRACE) << config->getInstanceName() << ": connect in progress'" << localAddress.toString() << "'";

                        enable(physicalSocket->getFd());
                    } else {
                        LOG(TRACE) << config->getInstanceName() << ": connect success '" << remoteAddress.toString() << "'";

                        SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(*physicalSocket, config);
                        onError(remoteAddress, state);
                    }

                    if (!isEnabled()) {
                        if (remoteAddress.useNext()) {
                            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                        } else {
                            onError(remoteAddress, state);
                        }

                        destruct();
                    }
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    LOG(ERROR) << config->getInstanceName() << ": " << badSocketAddress.what();

                    onError(remoteAddress, badSocketAddress.getState());
                    destruct();
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(ERROR) << config->getInstanceName() << ": " << badSocketAddress.what();

                onError(remoteAddress, badSocketAddress.getState());
                destruct();
            }
        } else {
            LOG(TRACE) << config->getInstanceName() << ": disabled";

            destruct();
        }
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectEvent() {
        int cErrno;

        core::socket::State state = core::socket::State::OK;

        if (physicalSocket->getSockError(cErrno) == 0) { //  == 0->return valid : < 0->getsockopt failed errno = cErrno;
            if (cErrno == 0) {
                LOG(TRACE) << config->getInstanceName() << ": connect success '" << remoteAddress.toString() << "'";

                onError(remoteAddress, state);
                disable();

                SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(*physicalSocket, config);
            } else if (!physicalSocket->connectInProgress(cErrno)) {
                LOG(TRACE) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                if (remoteAddress.useNext()) {
                    LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '"
                               << config->Remote::getSocketAddress().toString() << "'";

                    new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                } else {
                    utils::PreserveErrno pe(cErrno);

                    switch (cErrno) {
                        case EADDRINUSE:
                        case EADDRNOTAVAIL:
                        case ECONNREFUSED:
                        case ENETUNREACH:
                            PLOG(WARNING) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                            state = State::ERROR;
                            break;
                        default:
                            PLOG(ERROR) << config->getInstanceName() << ": connect failed '" << remoteAddress.toString() << "'";

                            state = State::FATAL;
                            break;
                    }

                    onError(remoteAddress, state);
                }

                disable();
            } else {
                // Connect still in progress
            }
        } else {
            PLOG(ERROR) << config->getInstanceName() << ": getsockopt syscall error '" << remoteAddress.toString() << "'";

            onError(remoteAddress, core::socket::State::FATAL);
            disable();
        }
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::destruct() {
        delete this;
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalSocketClient, typename Config, template <typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectTimeout() {
        LOG(WARNING) << config->getInstanceName() << ": connect timeout " << remoteAddress.toString();

        if (remoteAddress.useNext()) {
            LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '" << config->Remote::getSocketAddress().toString()
                       << "'";

            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
        } else {
            LOG(ERROR) << config->getInstanceName() << ": connect timeout '" << remoteAddress.toString() << "'";

            onError(remoteAddress, core::socket::State::ERROR);
        }

        disable();
    }

} // namespace core::socket::stream
