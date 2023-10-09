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
#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(
        const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::InitAcceptEventReceiver("SocketAcceptor")
        , core::eventreceiver::AcceptEventReceiver("SocketAcceptor", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onStatus(onStatus)
        , config(config) {
        InitAcceptEventReceiver::span();
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
        if (physicalSocket != nullptr) {
            delete physicalSocket;
        }
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::initAcceptEvent() {
        if (!config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << ": enabled";

            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());

            try {
                State state = State::OK;

                localAddress = config->Local::getSocketAddress();
                physicalSocket = new PhysicalSocket();

                if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                    switch (errno) {
                        case EMFILE:
                        case ENFILE:
                        case ENOBUFS:
                        case ENOMEM:
                            state = State::ERROR;
                            PLOG(WARNING) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = State::FATAL;
                            PLOG(ERROR) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalSocket->bind(localAddress) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = State::ERROR;
                            PLOG(WARNING) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = State::FATAL;
                            PLOG(ERROR) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalSocket->listen(config->getBacklog()) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = State::ERROR;
                            PLOG(WARNING) << config->getInstanceName() << ": listen '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = State::FATAL;
                            PLOG(ERROR) << config->getInstanceName() << ": listen '" << localAddress.toString() << "'";
                            break;
                    }
                } else {
                    enable(physicalSocket->getFd());
                    LOG(TRACE) << config->getInstanceName() << ": listen '" << localAddress.toString() << "' success";
                }

                if (localAddress.useNext()) {
                    LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '"
                               << config->Local::getSocketAddress().toString() << "'";
                    new SocketAcceptor(socketContextFactory, onConnect, onConnected, onDisconnect, onStatus, config);
                } else {
                    onStatus(localAddress, state);
                }

                if (!isEnabled()) {
                    destruct();
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(ERROR) << config->getInstanceName() << ": " << badSocketAddress.what();

                onStatus(localAddress, badSocketAddress.getState());
                destruct();
            }
        } else {
            LOG(TRACE) << config->getInstanceName() << ": disabled";

            onStatus(localAddress, core::socket::State::DISABLED);
            destruct();
        }
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::acceptEvent() {
        int acceptsPerTick = config->getAcceptsPerTick();

        do {
            PhysicalSocket physicalClientSocket(physicalSocket->accept4(PhysicalSocket::Flags::NONBLOCK));
            if (physicalClientSocket.isValid()) {
                LOG(TRACE) << config->getInstanceName() << ": accept success '" << localAddress.toString() << "'";

                SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(physicalClientSocket, config);
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(ERROR) << config->getInstanceName() << ": accept failed '" << localAddress.toString() << "'";
            }
        } while (--acceptsPerTick > 0);
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::destruct() {
        delete this;
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

} // namespace core::socket::stream
