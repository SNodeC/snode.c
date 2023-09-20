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

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    SocketConnector<PhysicalClientSocket, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, int)>& onError,
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

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    SocketConnector<PhysicalClientSocket, Config, SocketConnection>::~SocketConnector() {
        if (physicalSocket != nullptr) {
            delete physicalSocket;
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::initConnectEvent() {
        if (!config->getDisabled()) {
            core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());

            try {
                SocketAddress localAddress = config->Local::getSocketAddress();

                physicalSocket = new PhysicalSocket();
                remoteAddress = config->Remote::getSocketAddress();

                if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                    int errnum = errno;
                    PLOG(WARNING) << "SocketConnector::open '" << remoteAddress.toString() << "'";

                    if (localAddress.hasNext()) {
                        new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                    } else {
                        onError(remoteAddress, errnum);
                    }
                    destruct();
                } else if (physicalSocket->bind(localAddress) < 0) {
                    int errnum = errno;
                    PLOG(WARNING) << "SocketConnector::bind '" << remoteAddress.toString() << "'";

                    if (localAddress.hasNext()) {
                        new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                    } else {
                        onError(remoteAddress, errnum);
                    }
                    destruct();
                } else if (physicalSocket->connect(remoteAddress) < 0 && !physicalSocket->connectInProgress(errno)) {
                    int errnum = errno;
                    PLOG(WARNING) << "SocketConnector::connect '" << remoteAddress.toString() << "'";

                    if (remoteAddress.hasNext()) {
                        new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                    } else {
                        onError(remoteAddress, errnum);
                    }
                    destruct();
                } else {
                    enable(physicalSocket->getFd());
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(ERROR) << badSocketAddress.what();

                errno = badSocketAddress.getErrCode();
                onError(remoteAddress, errno);
                destruct();
            }
        } else {
            destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::connectEvent() {
        int cErrno;

        if (physicalSocket->getSockError(cErrno) == 0) { //  == 0->return valid : < 0->getsockopt failed errno = cErrno;
            errno = cErrno;
            if (physicalSocket->connectInProgress(errno)) {
                // Do nothing: connect() still in progress
            } else {
                disable();

                if (errno == 0) {
                    SocketConnectionFactory socketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect);
                    socketConnectionFactory.create(*physicalSocket, config);

                    onError(remoteAddress, errno);
                } else {
                    int errnum = errno;
                    PLOG(WARNING) << "SocketConnector::connectInProgress '" << remoteAddress.toString() << "'";

                    if (remoteAddress.hasNext()) {
                        new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
                    } else {
                        onError(remoteAddress, errnum);
                    }
                }
            }
        } else { // syscall error
            disable();
            onError(remoteAddress, errno);
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

        if (remoteAddress.hasNext()) {
            new SocketConnector(socketContextFactory, onConnect, onConnected, onDisconnect, onError, config);
        } else {
            errno = ETIMEDOUT;
            onError(remoteAddress, errno);
        }
    }

} // namespace core::socket::stream
