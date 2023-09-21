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

#include "core/ProgressLog.h"
#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::SocketAcceptor(
        const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, int)>& onError,
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<core::ProgressLog> progressLog)
        : core::eventreceiver::InitAcceptEventReceiver("SocketAcceptor")
        , core::eventreceiver::AcceptEventReceiver("SocketAcceptor", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onError(onError)
        , config(config)
        , progressLog(progressLog) {
        InitAcceptEventReceiver::span();
    }

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::~SocketAcceptor() {
        if (physicalSocket != nullptr) {
            delete physicalSocket;
        }
    }

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::initAcceptEvent() {
        if (!config->getDisabled()) {
            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());

            try {
                localAddress = config->Local::getSocketAddress();
                physicalSocket = new PhysicalSocket();

                int errnum = 0;

                if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                    progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketAcceptor::open '" << localAddress.toString()
                                             << "'";
                    errnum = errno;
                    PLOG(WARNING) << "SocketAcceptor::open '" << localAddress.toString() << "'";
                } else if (physicalSocket->bind(localAddress) < 0) {
                    progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketAcceptor::bind '" << localAddress.toString()
                                             << "'";
                    errnum = errno;
                    PLOG(WARNING) << "SocketAcceptor::bind '" << localAddress.toString() << "'";

                } else if (physicalSocket->listen(config->getBacklog()) < 0) {
                    progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketAcceptor::listen '" << localAddress.toString()
                                             << "'";
                    errnum = errno;
                    PLOG(WARNING) << "SocketAcceptor::listen '" << localAddress.toString() << "'";
                } else {
                    progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketAcceptor::listen '" << localAddress.toString()
                                             << "'";
                    errnum = 0;
                    enable(physicalSocket->getFd());
                }

                if (localAddress.useNext()) {
                    new SocketAcceptor(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config, progressLog);
                } else {
                    onError(localAddress, errnum);
                    progressLog->logProgress();
                }

                if (!isEnabled()) {
                    destruct();
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(ERROR) << badSocketAddress.what();

                errno = badSocketAddress.getErrCode();
                onError(localAddress, errno);
                destruct();
            }
        } else {
            destruct();
        }
    }

    template <typename PhysicalServerSocket, typename Config, template <typename PhysicalServerSocketT> typename SocketConnection>
    void SocketAcceptor<PhysicalServerSocket, Config, SocketConnection>::acceptEvent() {
        int acceptsPerTick = config->getAcceptsPerTick();

        do {
            PhysicalSocket physicalClientSocket(physicalSocket->accept4(PhysicalSocket::Flags::NONBLOCK));
            if (physicalClientSocket.isValid()) {
                SocketConnectionFactory socketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect);
                socketConnectionFactory.create(physicalClientSocket, config);
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(ERROR) << "SocketAcceptor::accept '" << localAddress.toString() << "'";
            }
        } while (--acceptsPerTick > 0);
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
