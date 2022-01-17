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

    template <typename SocketConnectionT>
    class SocketConnector
        : protected SocketConnectionT::Socket
        , protected ConnectEventReceiver {
        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;
        SocketConnector& operator=(const SocketConnector&) = delete;

    public:
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
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

        void connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int)>& onError) {
            this->onError = onError;

            Socket::open(
                [this, &bindAddress, &remoteAddress, &onError](int errnum) -> void {
                    if (errnum > 0) {
                        onError(errnum);
                        destruct();
                    } else {
                        Socket::bind(bindAddress, [this, &remoteAddress, &onError](int errnum) -> void {
                            if (errnum > 0) {
                                onError(errnum);
                                destruct();
                            } else {
                                int ret = core::system::connect(Socket::fd, &remoteAddress.getSockAddr(), remoteAddress.getSockAddrLen());

                                if (ret == 0 || errno == EINPROGRESS) {
                                    enable(Socket::fd);
                                } else {
                                    onError(errno);
                                    destruct();
                                }
                            }
                        });
                    }
                },
                SOCK_NONBLOCK);
        }

    private:
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

                        if (core::system::getsockname(Socket::fd, reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) == 0 &&
                            core::system::getpeername(Socket::fd, reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) == 0) {
                            SocketConnection* socketConnection = new SocketConnection(SocketConnector::getFd(),
                                                                                      socketContextFactory,
                                                                                      SocketAddress(localAddress),
                                                                                      SocketAddress(remoteAddress),
                                                                                      onConnect,
                                                                                      onDisconnect);

                            onConnected(socketConnection);
                            onError(0);

                            Socket::dontClose(true);
                            disable();
                        } else {
                            onError(errno);
                            disable();
                        }
                    } else {
                        onError(errno);
                        disable();
                    }
                } else {
                    // Do nothing: connect() still in progress
                }
            } else {
                onError(errno);
                disable();
            }
        }

        void unobservedEvent() override {
            destruct();
        }

    protected:
        void destruct() {
            delete this;
        }

    private:
        std::shared_ptr<core::socket::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onDestruct;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

    protected:
        std::function<void(int err)> onError;

        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
