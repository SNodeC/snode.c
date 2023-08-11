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

#include "core/socket/stream/SocketConnectionFactory.hpp"
#include "core/socket/stream/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    core::socket::stream::SocketConnector<PhysicalClientSocket, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, int)>& onError,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::InitConnectEventReceiver("SocketConnector")
        , core::eventreceiver::ConnectEventReceiver("SocketConnector")
        , socketConnectionFactory(socketContextFactory, onConnect, onConnected, onDisconnect)
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
            try {
                physicalSocket = new PhysicalSocket();
                localAddress = config->Local::getSocketAddress();
                remoteAddress = config->Remote::getSocketAddress();

                if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                    onError(remoteAddress, errno);
                    VLOG(0) << "############################### 1";
                    destruct();
                } else if (physicalSocket->bind(localAddress) < 0) {
                    onError(remoteAddress, errno);
                    VLOG(0) << "############################### 2";
                    destruct();
                    if (localAddress.hasNext()) {
                    }
                } else if (physicalSocket->connect(remoteAddress) < 0 && !physicalSocket->connectInProgress(errno)) {
                    onError(remoteAddress, errno);
                    VLOG(0) << "############################### 3";
                    destruct();
                    if (remoteAddress.hasNext()) {
                    }
                } else {
                    enable(physicalSocket->getFd());
                }
            } catch (const SocketAddress::BadSocketAddress& badSocketAddress) {
                errno = badSocketAddress.getErrCode();
                LOG(ERROR) << badSocketAddress.what();
                onError(remoteAddress, errno);
                destruct();
            }
        } else {
            destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::connectEvent() {
        int cErrno = -1;

        if ((cErrno = physicalSocket->getSockError()) >= 0) { //  >= 0->return valid : < 0->getsockopt failed errno = cErrno;
            if (!physicalSocket->connectInProgress(cErrno)) {
                if (cErrno == 0) {
                    disable();
                    if (socketConnectionFactory.create(*physicalSocket, config)) {
                        errno = errno == 0 ? cErrno : errno;
                        VLOG(0) << "############################### 4";
                        onError(remoteAddress, errno);
                    }
                } else {
                    disable();
                    errno = cErrno;
                    VLOG(0) << "############################### 5";
                    onError(remoteAddress, errno);
                    if (remoteAddress.hasNext()) {
                    }
                }
            } else {
                // Do nothing: connect() still in progress
            }
        } else {
            disable();
            errno = cErrno;
            VLOG(0) << "############################### 6";
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

} // namespace core::socket::stream
