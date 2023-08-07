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
            physicalSocket = new PhysicalSocket();
            if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                onError(config->Remote::getSocketAddress(), errno);
                destruct();
            } else if (physicalSocket->bind(config->Local::getSocketAddress()) < 0) {
                onError(config->Remote::getSocketAddress(), errno);
                destruct();
            } else if (physicalSocket->connect(config->Remote::getSocketAddress()) < 0 && !physicalSocket->connectInProgress(errno)) {
                onError(config->Remote::getSocketAddress(), errno);
                destruct();
            } else {
                enable(physicalSocket->getFd());
            }
        } else {
            destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config, template <typename PhysicalClientSocketT> typename SocketConnection>
    void SocketConnector<PhysicalClientSocket, Config, SocketConnection>::connectEvent() {
        int cErrno = -1;

        int tmpErrno = errno;
        errno = tmpErrno;
        if ((cErrno = physicalSocket->getSockError()) >= 0) { //  >= 0->return valid : < 0->getsockopt failed errno = cErrno;
            tmpErrno = errno;
            errno = tmpErrno;
            if (!physicalSocket->connectInProgress(cErrno)) {
                tmpErrno = errno;
                errno = tmpErrno;
                if (cErrno == 0) {
                    tmpErrno = errno;
                    errno = tmpErrno;
                    disable();
                    if (socketConnectionFactory.create(*physicalSocket, config)) {
                        errno = errno == 0 ? cErrno : errno;
                        onError(config->Remote::getSocketAddress(), errno);
                    }
                } else {
                    disable();
                    errno = cErrno;
                    onError(config->Remote::getSocketAddress(), errno);
                }
            } else {
                tmpErrno = errno;
                errno = tmpErrno;
                // Do nothing: connect() still in progress
            }
        } else {
            tmpErrno = errno;
            errno = tmpErrno;
            disable();
            errno = cErrno;
            // Try the next sockaddr (IPv4, IPv6)
            onError(config->Remote::getSocketAddress(), errno);
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
