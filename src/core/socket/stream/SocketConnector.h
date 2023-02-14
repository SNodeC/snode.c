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

#ifndef CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_SOCKETCONNECTOR_H

#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/stream/SocketConnectionFactory.h"
#include "net/config/ConfigPhysicalSocket.h"

namespace core::socket::stream {
    class SocketContextFactory;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketClientT, template <typename PhysicalSocket> class SocketConnectionT>
    class SocketConnector
        : protected core::eventreceiver::InitConnectEventReceiver
        , protected core::eventreceiver::ConnectEventReceiver {
    private:
        using SocketClient = SocketClientT;
        using PhysicalSocket = typename SocketClient::PhysicalSocket;

    protected:
        using SocketConnection = SocketConnectionT<PhysicalSocket>;
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<SocketClient, SocketConnection>;

    public:
        using Config = typename SocketClient::Config;
        using SocketAddress = typename SocketClient::SocketAddress;

        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;

        SocketConnector& operator=(const SocketConnector&) = delete;

        SocketConnector(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
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

        ~SocketConnector() override {
            if (physicalSocket != nullptr) {
                delete physicalSocket;
            }
        }

    protected:
        void initConnectEvent() override {
            if (!config->getDisabled()) {
                physicalSocket = new PhysicalSocket();
                if (physicalSocket->open(config->getSocketOptions(), PhysicalSocket::Flags::NONBLOCK) < 0) {
                    onError(config->getRemoteAddress(), errno);
                    destruct();
                } else if (physicalSocket->bind(config->getLocalAddress()) < 0) {
                    onError(config->getRemoteAddress(), errno);
                    destruct();
                } else if (physicalSocket->connect(config->getRemoteAddress()) < 0 && !physicalSocket->connectInProgress(errno)) {
                    onError(config->getRemoteAddress(), errno);
                    destruct();
                } else {
                    enable(physicalSocket->getFd());
                }
            } else {
                destruct();
            }
        }

    private:
        void connectEvent() override {
            int cErrno = -1;

            if ((cErrno = physicalSocket->getSockError()) >= 0) { //  >= 0->return valid : < 0->getsockopt failed errno = cErrno;
                if (!physicalSocket->connectInProgress(cErrno)) {
                    if (cErrno == 0) {
                        disable();
                        if (socketConnectionFactory.create(*physicalSocket, config)) {
                            errno = errno == 0 ? cErrno : errno;
                            onError(config->getRemoteAddress(), errno);
                        }
                    } else {
                        disable();
                    }
                } else {
                    // Do nothing: connect() still in progress
                }
            } else {
                disable();
                onError(config->getRemoteAddress(), cErrno);
            }
        }

    protected:
        void destruct() {
            delete this;
        }

    private:
        void unobservedEvent() override {
            destruct();
        }

        PhysicalSocket* physicalSocket = nullptr;

        SocketConnectionFactory socketConnectionFactory;

    protected:
        std::function<void(const SocketAddress& socketAddress, int err)> onError;

        std::shared_ptr<Config> config = nullptr;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
