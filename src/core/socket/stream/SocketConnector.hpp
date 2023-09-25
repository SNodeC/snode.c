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
#include "core/socket/stream/SocketConnectionFactory.hpp" // IWYU pragma: export
#include "core/socket/stream/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    SocketConnector<PhysicalClientSocket, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const core::ProgressLog&)>& onError,
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<core::ProgressLog> progressLog)
        : core::eventreceiver::InitConnectEventReceiver("SocketConnector")
        , core::eventreceiver::ConnectEventReceiver("SocketConnector", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onError(onError)
        , config(config)
        , progressLog(progressLog) {
        InitConnectEventReceiver::span();
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    SocketConnector<PhysicalClientSocket, Config, SocketConnection>::~SocketConnector() {
        if (physicalSocket != nullptr) {
            delete physicalSocket;
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::initConnectEvent() {
        if (!config->getDisabled()) {
            LOG(INFO) << "Instance '" << this->config->getInstanceName() << "' enabled";

            core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());

            SocketAddress localAddress;

            try {
                localAddress = config->Local::getSocketAddress();

                try {
                    remoteAddress = config->Remote::getSocketAddress();

                    physicalSocket = new PhysicalSocket();

                    if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                        progressLog->addEntry(0)
                            << this->config->getInstanceName() << ": SocketConnector::open '" << remoteAddress.toString() << "'";
                    } else if (physicalSocket->bind(localAddress) < 0) {
                        progressLog->addEntry(0)
                            << this->config->getInstanceName() << ": SocketConnector::bind '" << remoteAddress.toString() << "'";
                    } else if (physicalSocket->connect(remoteAddress) != 0 && !physicalSocket->connectInProgress(errno)) {
                        progressLog->addEntry(0)
                            << this->config->getInstanceName() << ": SocketConnector::initial connect '" << remoteAddress.toString() << "'";
                    } else if (physicalSocket->connectInProgress(errno)) {
                        utils::PreserveErrno pe(0);
                        progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::initial connect '"
                                                 << remoteAddress.toString() << "' connecting in progress";

                        enable(physicalSocket->getFd());
                    } else {
                        progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::initial connect '"
                                                 << remoteAddress.toString() << "' connection estableshed";
                        onError(*progressLog);

                        SocketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect).create(*physicalSocket, config);
                    }

                    if (!isEnabled()) {
                        if (remoteAddress.useNext()) {
                            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config, progressLog);
                        } else {
                            onError(*progressLog);
                        }

                        destruct();
                    }
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    utils::PreserveErrno pe(0);
                    progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::getRemoteAddress '"
                                             << remoteAddress.toString() << "': BadSocketAddress: " << badSocketAddress.what();
                    onError(*progressLog);

                    destruct();
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                utils::PreserveErrno pe(0);
                progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::getLocalAddress '"
                                         << localAddress.toString() << "': BadSocketAddress: " << badSocketAddress.what();
                onError(*progressLog);

                destruct();
            }
        } else {
            LOG(INFO) << "Instance '" << this->config->getInstanceName() << "' disabled";

            destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::connectEvent() {
        int cErrno;

        if (physicalSocket->getSockError(cErrno) == 0) { //  == 0->return valid : < 0->getsockopt failed errno = cErrno;
            errno = cErrno;

            if (errno == 0) {
                progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::connectEvent '"
                                         << remoteAddress.toString() << "'";
                onError(*progressLog.get());

                disable();

                SocketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect).create(*physicalSocket, config);
            } else if (!physicalSocket->connectInProgress(errno)) {
                progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::connectEvent::error '"
                                         << remoteAddress.toString() << "'";
                if (remoteAddress.useNext()) {
                    new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config, progressLog);
                } else {
                    onError(*progressLog);
                }

                disable();
            }
        } else { // syscall error
            progressLog->addEntry(0) << this->config->getInstanceName() << ": SocketConnector::connectEvent '" << remoteAddress.toString()
                                     << "'";
            onError(*progressLog);

            disable();
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::destruct() {
        delete this;
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::connectTimeout() {
        disable();

        if (remoteAddress.useNext()) {
            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
        } else {
            errno = ETIMEDOUT;
            onError(*progressLog);
        }
    }

} // namespace core::socket::stream
