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

#include "core/ConnectEventReceiver.h"
#include "core/InitConnectEventReceiver.h"

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

    template <typename ClientConfigT, typename SocketConnectionT>
    class SocketConnector
        : protected SocketConnectionT::Socket
        , protected InitConnectEventReceiver
        , protected ConnectEventReceiver {
        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;
        SocketConnector& operator=(const SocketConnector&) = delete;

    protected:
        using ClientConfig = ClientConfigT;
        using SocketConnection = SocketConnectionT;

    private:
        using Socket = typename SocketConnection::Socket;

    public:
        using SocketAddress = typename Socket::SocketAddress;

        SocketConnector(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options)
            : socketContextFactory(socketContextFactory)
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        virtual ~SocketConnector() = default;

        void connect(const std::shared_ptr<ClientConfig>& clientConfig,
                     const std::function<void(const SocketAddress&, int)>& onError) {
            this->clientConfig = clientConfig;
            this->onError = onError;

            InitConnectEventReceiver::publish();
        }

    private:
        void initConnectEvent() override {
            Socket::open(
                [this](int errnum) -> void {
                    if (errnum > 0) {
                        onError(clientConfig->getRemoteAddress(), errnum);
                        destruct();
                    } else {
                        Socket::bind(clientConfig->getLocalAddress(), [this](int errnum) -> void {
                            if (errnum > 0) {
                                onError(clientConfig->getRemoteAddress(), errnum);
                                destruct();
                            } else {
                                int ret = core::system::connect(Socket::getFd(),
                                                                &clientConfig->getRemoteAddress().getSockAddr(),
                                                                clientConfig->getRemoteAddress().getSockAddrLen());

                                if (ret == 0 || errno == EINPROGRESS) {
                                    enable(Socket::getFd());
                                    onError(clientConfig->getRemoteAddress(), 0);
                                } else {
                                    onError(clientConfig->getRemoteAddress(), errno);
                                    destruct();
                                }
                            }
                        });
                    }
                },
                SOCK_NONBLOCK);
        }

        void connectEvent() override {
            int cErrno = -1;
            socklen_t cErrnoLen = sizeof(cErrno);

            int err = core::system::getsockopt(SocketConnector::getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

            if (err == 0) {
                errno = cErrno;
                if (errno != EINPROGRESS) {
                    if (errno == 0) {
                        typename SocketAddress::SockAddr localAddress{};
                        socklen_t localAddressLength = sizeof(localAddress);

                        typename SocketAddress::SockAddr remoteAddress{};
                        socklen_t remoteAddressLength = sizeof(remoteAddress);

                        if (core::system::getsockname(Socket::getFd(), reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) ==
                                0 &&
                            core::system::getpeername(Socket::getFd(), reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) ==
                                0) {
                            SocketConnection* socketConnection = new SocketConnection(SocketConnector::getFd(),
                                                                                      socketContextFactory,
                                                                                      SocketAddress(localAddress),
                                                                                      SocketAddress(remoteAddress),
                                                                                      onConnect,
                                                                                      onDisconnect,
                                                                                      clientConfig->getReadTimeout(),
                                                                                      clientConfig->getWriteTimeout(),
                                                                                      clientConfig->getReadBlockSize(),
                                                                                      clientConfig->getWriteBlockSize(),
                                                                                      clientConfig->getTerminateTimeout());

                            onConnected(socketConnection);
                            onError(clientConfig->getRemoteAddress(), 0);

                            Socket::dontClose(true);
                            disable();
                        } else {
                            onError(clientConfig->getRemoteAddress(), errno);
                            disable();
                        }
                    } else {
                        onError(clientConfig->getRemoteAddress(), errno);
                        disable();
                    }
                } else {
                    // Do nothing: connect() still in progress
                }
            } else {
                onError(clientConfig->getRemoteAddress(), errno);
                disable();
            }
        }

    protected:
        void destruct() {
            delete this;
        }

        std::shared_ptr<ClientConfig> clientConfig = nullptr;

    private:
        void unobservedEvent() override {
            destruct();
        }

        std::shared_ptr<core::socket::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onDestruct;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

    protected:
        std::function<void(const SocketAddress& socketAddress, int err)> onError;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
