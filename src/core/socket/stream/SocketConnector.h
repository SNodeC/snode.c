/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include "core/eventreceiver/InitConnectEventReceiver.h"
#include "core/socket/stream/SocketConnectionEstablisher.h"

namespace core::socket {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename ClientSocketT, template <typename SocketT> class SocketConnectionT>
    class SocketConnector
        : protected ClientSocketT::Socket
        , protected core::eventreceiver::InitConnectEventReceiver
        , protected core::eventreceiver::ConnectEventReceiver {
        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;
        SocketConnector& operator=(const SocketConnector&) = delete;

    private:
        using ClientSocket = ClientSocketT;
        using Socket = typename ClientSocket::Socket;

    protected:
        using SocketConnection = SocketConnectionT<Socket>;
        using SocketConnectionEstablisher = core::socket::stream::SocketConnectionEstablisher<ClientSocketT, SocketConnectionT>;

    public:
        using Config = typename ClientSocket::Config;
        using SocketAddress = typename ClientSocket::SocketAddress;

        SocketConnector(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options)
            : core::eventreceiver::InitConnectEventReceiver("SocketConnector")
            , core::eventreceiver::ConnectEventReceiver("SocketConnector")
            , socketConnectionEstablisher(socketContextFactory, onConnect, onConnected, onDisconnect)
            , options(options) {
        }

        ~SocketConnector() override = default;

        void connect(const std::shared_ptr<Config>& clientConfig, const std::function<void(const SocketAddress&, int)>& onError) {
            this->config = clientConfig;
            this->onError = onError;

            InitConnectEventReceiver::publish();
        }

    private:
        void initConnectEvent() override {
            if (Socket::open(SOCK_NONBLOCK) < 0) {
                onError(config->getRemoteAddress(), errno);
                destruct();
            } else if (Socket::bind(config->getLocalAddress()) < 0) {
                onError(config->getRemoteAddress(), errno);
                destruct();
            } else if (core::system::connect(
                           Socket::getFd(), &config->getRemoteAddress().getSockAddr(), config->getRemoteAddress().getSockAddrLen()) < 0 &&
                       !Socket::connectInProgress()) {
                onError(config->getRemoteAddress(), errno);
                destruct();
            } else {
                onError(config->getLocalAddress(), 0);
                enable(Socket::getFd());
            }
        }

        void connectEvent() override {
            int cErrno = -1;
            socklen_t cErrnoLen = sizeof(cErrno);

            int err = core::system::getsockopt(SocketConnector::getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

            if (err == 0) {
                errno = cErrno;
                if (!Socket::connectInProgress()) {
                    if (errno == 0) {
                        disable();

                        typename SocketAddress::SockAddr localAddress{};
                        socklen_t localAddressLength = sizeof(localAddress);

                        typename SocketAddress::SockAddr remoteAddress{};
                        socklen_t remoteAddressLength = sizeof(remoteAddress);

                        if (core::system::getsockname(Socket::getFd(), reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) ==
                                0 &&
                            core::system::getpeername(Socket::getFd(), reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) ==
                                0) {
                            socketConnectionEstablisher.establishConnection(Socket::getFd(), localAddress, remoteAddress, config);

                            Socket::dontClose(true);
                        } else {
                            onError(config->getRemoteAddress(), errno);
                            disable();
                        }
                    } else {
                        onError(config->getRemoteAddress(), errno);
                        disable();
                    }
                } else {
                    // Do nothing: connect() still in progress
                }
            } else {
                onError(config->getRemoteAddress(), errno);
                disable();
            }
        }

    protected:
        void destruct() {
            delete this;
        }

        std::shared_ptr<Config> config = nullptr;

    private:
        void unobservedEvent() override {
            destruct();
        }

    protected:
        std::function<void(const SocketAddress& socketAddress, int err)> onError;

        SocketConnectionEstablisher socketConnectionEstablisher;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
