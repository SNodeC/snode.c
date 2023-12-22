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

#include <utility>

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
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::initAcceptEvent() {
        if (!config->getDisabled()) {
            try {
                LOG(TRACE) << config->getInstanceName() << ": starting";

                SocketAddress localAddress(config->Local::getSocketAddress());

                core::socket::State state = core::socket::STATE_OK;

                if (physicalServerSocket.open(config->getSocketOptions(), PhysicalServerSocket::Flags::NONBLOCK) < 0) {
                    switch (errno) {
                        case EMFILE:
                        case ENFILE:
                        case ENOBUFS:
                        case ENOMEM:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << ": open '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalServerSocket.bind(localAddress) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << ": bind '" << localAddress.toString() << "'";
                            break;
                    }
                } else if (physicalServerSocket.listen(config->getBacklog()) < 0) {
                    switch (errno) {
                        case EADDRINUSE:
                            state = core::socket::STATE_ERROR;
                            PLOG(TRACE) << config->getInstanceName() << ": listen '" << localAddress.toString() << "'";
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            PLOG(TRACE) << config->getInstanceName() << ": listen '" << localAddress.toString() << "'";
                            break;
                    }
                } else {
                    LOG(TRACE) << config->getInstanceName() << ": listen '" << localAddress.toString() << "' success";

                    enable(physicalServerSocket.getFd());
                }

                if (localAddress.useNext()) {
                    LOG(TRACE) << config->getInstanceName() << ": using next SocketAddress '"
                               << config->Local::getSocketAddress().toString() << "'";

                    new SocketAcceptor(socketContextFactory, onConnect, onConnected, onDisconnect, onStatus, config);
                } else {
                    onStatus(localAddress, state);
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
            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketServer, typename Config, template <typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::acceptEvent() {
        int acceptsPerTick = config->getAcceptsPerTick();

        do {
            PhysicalServerSocket physicalClientSocket(physicalServerSocket.accept4(PhysicalServerSocket::Flags::NONBLOCK),
                                                      physicalServerSocket.getBindAddress());
            if (physicalClientSocket.isValid()) {
                LOG(TRACE) << config->getInstanceName() << ": accept success '" << physicalServerSocket.getBindAddress().toString() << "'";

                SocketConnectionFactory(onConnect, onConnected, onDisconnect).create(std::move(physicalClientSocket), config);
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(TRACE) << config->getInstanceName() << ": accept failed '" << physicalServerSocket.getBindAddress().toString() << "'";
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
